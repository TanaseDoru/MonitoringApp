#include "utils.h"
#include "processHandler.h"
#include <ncurses.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <signal.h>
#include <pwd.h>
#include <pthread.h>

#define WIDTH 90
#define HEIGHT_TOP 9
#define HEIGHT_PROC 30
#define MAX_PROCESSES 400
#define MAX_THREADS 4
#define BUFFER_SIZE 1024
#define HEIGHT_OPT 8
#define SECONDS 2

int nr_processes = 0;

typedef struct {
    int pid;
    int ppid;
    char user[BUFFER_SIZE];
    char state;
    char name[BUFFER_SIZE];
    int nice;
    float cpu_usage;
    char args[BUFFER_SIZE];
} Process;

typedef struct {
    Process* proc;
    char** pid_list;
    int start;
    int end;
} ThreadData;

typedef struct {
    double total_cpu_usage;
    struct sysinfo sys_info;
    char current_time[64];
    double load_avg[3];
    unsigned long used_ram, total_ram;
    unsigned long used_swap, total_swap;
}SummaryData;

typedef enum {
    None,
    Pid=1,
    PPid,
    User,
    Nice,
    Pid_desc=-1,
    PPid_desc=-2,
    User_desc=-3,
    Nice_desc=-4
}sorting_state;

typedef struct {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} CpuTimes;

int compare_by_pid(const void* a, const void* b) {
    return ((Process*)a)->pid - ((Process*)b)->pid;
}

int compare_by_ppid(const void* a, const void* b) {
    return ((Process*)a)->ppid - ((Process*)b)->ppid;
}

int compare_by_user(const void* a, const void* b) {
    return strcmp(((Process*)a)->user, ((Process*)b)->user);
}

int compare_by_nice(const void* a, const void* b) {
    return ((Process*)a)->nice - ((Process*)b)->nice;
}

int compare_by_name(const void* a, const void* b) {
    return strcmp(((Process*)a)->name, ((Process*)b)->name);
}

int compare_by_pid_desc(const void* a, const void* b) {
    return -((Process*)a)->pid + ((Process*)b)->pid;
}

int compare_by_ppid_desc(const void* a, const void* b) {
    return -((Process*)a)->ppid + ((Process*)b)->ppid;
}

int compare_by_user_desc(const void* a, const void* b) {
    return -strcmp(((Process*)a)->user, ((Process*)b)->user);
}

int compare_by_nice_desc(const void* a, const void* b) {
    return -((Process*)a)->nice + ((Process*)b)->nice;
}

int compare_by_name_desc(const void* a, const void* b) {
    return -strcmp(((Process*)a)->name, ((Process*)b)->name);
}

int is_number(const char* buffer) {
    for (int i = 0; i < strlen(buffer); i++) {
        if (!isdigit(buffer[i]))
            return 0;
    }
    return 1;
}

char* get_username(int uid) {
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        char username[BUFFER_SIZE];
        if (username) {
            strcpy(username, pw->pw_name);
        }
        return strdup(username);
    }
    return NULL;
}

void get_process_by_pid(char* pid, Process* current_proc) {
    char filename[BUFFER_SIZE];
    snprintf(filename, sizeof(filename), "/proc/%s/status", pid);

    FILE* f = fopen(filename, "r");
    if (!f) return;

    char buffer[BUFFER_SIZE];
    current_proc->pid = atoi(pid);

    while (fgets(buffer, sizeof(buffer), f) != NULL) {
        if (strncmp(buffer, "Name:", 5) == 0) {
            sscanf(buffer, "Name:\t%s", current_proc->name);
        } else if (strncmp(buffer, "State:", 6) == 0) {
            sscanf(buffer, "State:\t%c", &current_proc->state);
        } else if (strncmp(buffer, "PPid:", 5) == 0) {
            sscanf(buffer, "PPid:\t%d", &current_proc->ppid);
        } else if (strncmp(buffer, "Uid:", 4) == 0) {
            int uid;
            sscanf(buffer, "Uid:\t%d", &uid);
            char* username = get_username(uid);
            if (username) {
                strcpy(current_proc->user, username);
                free(username);
            }
        }
    }

    fclose(f);

    snprintf(filename, sizeof(filename), "/proc/%s/cmdline", pid);
    f = fopen(filename, "r");
    if (!f) return;

    strcpy(buffer, "");

    if (fread(buffer, sizeof(char), sizeof(buffer), f))
    {
        strcpy(current_proc->args, buffer);
    }

    fclose(f);

    snprintf(filename, sizeof(filename), "/proc/%s/stat", pid);
    f = fopen(filename, "r");
    if (!f) return;

    unsigned long utime, stime, starttime;
    long nice;

    if (fgets(buffer, sizeof(buffer), f) != NULL) {
        int pid;
        char comm[256];
        char state;
        int ppid, pgrp, session, tty_nr, tpgid;
        unsigned flags;
        unsigned long minflt, cminflt, majflt, cmajflt, vsize;
        long cutime, cstime, priority, num_threads, itrealvalue, rss;

        sscanf(buffer,
           "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %ld %lu",
           &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
           &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, &cstime,
           &priority, &nice, &num_threads, &itrealvalue, &starttime, &vsize);

        current_proc->nice = (int)nice;

        struct sysinfo sys_info;
        if (sysinfo(&sys_info) == 0) {
            double uptime;
            FILE *uptime_file = fopen("/proc/uptime", "r");
            if (uptime_file != NULL) {
                fscanf(uptime_file, "%lf", &uptime);
                fclose(uptime_file); 
            } else { 
                uptime = sys_info.uptime;
            }

            long clk_tck = sysconf(_SC_CLK_TCK);
            double total_time = (utime + stime) / (double)clk_tck;
            double seconds = uptime - (starttime / (double)clk_tck);
            if (seconds > 0) {
                current_proc->cpu_usage = 100.0 * (total_time / seconds);
            } else {
                current_proc->cpu_usage = 0.0;
            }
        }
    }

    fclose(f);
}

void* process_range(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    Process* proc = data->proc;
    char** pid_list = data->pid_list;
    int start = data->start;
    int end = data->end;

    for (int i = start; i < end; i++) {
        get_process_by_pid(pid_list[i], &proc[i]);
    }

    return NULL;
}

void get_processes(Process* proc) {
    
    DIR* proc_dir = opendir("/proc"); 
    struct dirent* entry;
    char* pid_list[MAX_PROCESSES];
    int proc_count = 0;

    if (!proc_dir) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(proc_dir)) != NULL) {
        if (is_number(entry->d_name)) {
            pid_list[proc_count] = strdup(entry->d_name);
            proc_count++;
            if (proc_count >= MAX_PROCESSES) break;
        }
    }
    closedir(proc_dir);
    nr_processes = proc_count;

    int threads_count = MAX_THREADS;
    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];

    int chunk_size = (proc_count + threads_count - 1) / threads_count;

    for (int i = 0; i < threads_count; i++) {
        thread_data[i].proc = proc;
        thread_data[i].pid_list = pid_list;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i + 1) * chunk_size;
        if (thread_data[i].end > proc_count) thread_data[i].end = proc_count;
        pthread_create(&threads[i], NULL, process_range, &thread_data[i]);
    }

    for (int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < proc_count; i++) {
        free(pid_list[i]);
    }
}

void display_processes_info(WINDOW* proc_win, int start_index, int highlight, Process* proc, sorting_state ss, char* search_buffer, int* display_count, 
                            int* selected_pid, int right_pressed) {
    
    switch (ss)
    {
        case Pid:
            qsort(proc, nr_processes, sizeof(Process), compare_by_pid);
        break;
        case PPid:
            qsort(proc, nr_processes, sizeof(Process), compare_by_ppid);
        break;
        case Nice:
            qsort(proc, nr_processes, sizeof(Process), compare_by_nice);
        break;
        case User:
            qsort(proc, nr_processes, sizeof(Process), compare_by_user);
        break;  
        case Pid_desc:
            qsort(proc, nr_processes, sizeof(Process), compare_by_pid_desc);
        break;
        case PPid_desc:
            qsort(proc, nr_processes, sizeof(Process), compare_by_ppid_desc);
        break;
        case Nice_desc:
            qsort(proc, nr_processes, sizeof(Process), compare_by_nice_desc);
        break;
        case User_desc:
            qsort(proc, nr_processes, sizeof(Process), compare_by_user_desc);
        break;  
        default:
        break;
    }


    char buffer[BUFFER_SIZE];
    if(right_pressed)
    {
        mvwprintw(proc_win, 1, 2, "\t\t\tCMD\tARGS");
    }
    else
    {
        mvwprintw(proc_win, 1, 2, "S\tOwner\tPID\tPPID\tNice\tCPU\tCMD");
    }

    *display_count = 0;
    int current_process = start_index;

    for (int i = 0; i < nr_processes; i++) {
        if (*display_count >= HEIGHT_PROC - 3) break;

        if (strcmp(search_buffer, "") == 0 || 
            strstr(proc[current_process].name, search_buffer) != NULL ||
            strstr(proc[current_process].user, search_buffer) != NULL ||
            strstr(proc[current_process].args, search_buffer) != NULL) {
            
            if (*display_count == highlight)
                {
                    wattron(proc_win, A_REVERSE);
                    *selected_pid = proc[current_process].pid;
                }

            if(right_pressed)
            {
                mvwprintw(proc_win, *display_count + 2, 2, "%26s\t%s", 
                    proc[current_process].name, 
                    proc[current_process].args);
            }
            else
            {
                mvwprintw(proc_win, *display_count + 2, 2, "%c\t%s\t%d\t%d\t%d\t%.2f\t%s", 
                    proc[current_process].state, 
                    proc[current_process].user, 
                    proc[current_process].pid, 
                    proc[current_process].ppid, 
                    proc[current_process].nice, 
                    proc[current_process].cpu_usage, 
                    proc[current_process].name);
            }
            
            
            if (*display_count == highlight)
                wattroff(proc_win, A_REVERSE);

            (*display_count)++;
        }
        current_process++;
    }
    box(proc_win, 0, 0);
    wrefresh(proc_win);
}


double calculate_cpu_usage(CpuTimes* prev, CpuTimes* curr) {
    unsigned long long prev_idle = prev->idle + prev->iowait;
    unsigned long long curr_idle = curr->idle + curr->iowait;

    unsigned long long prev_non_idle = prev->user + prev->nice + prev->system + prev->irq + prev->softirq + prev->steal;
    unsigned long long curr_non_idle = curr->user + curr->nice + curr->system + curr->irq + curr->softirq + curr->steal;

    unsigned long long prev_total = prev_idle + prev_non_idle;
    unsigned long long curr_total = curr_idle + curr_non_idle;

    unsigned long long total_diff = curr_total - prev_total;
    unsigned long long idle_diff = curr_idle - prev_idle;

    return 100.0 * (double)(total_diff - idle_diff) / (double)total_diff;
}

void get_cpu_times(CpuTimes* times) {
    FILE * fp = fopen("/proc/stat", "r");

    if (fp == NULL) {
        return;
    }
    
    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp); 

    sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu\n",
        &times->user, &times->nice, &times->system, &times->idle,
        &times->iowait, &times->irq, &times->softirq, &times->steal);
}

void get_system_info(struct sysinfo* sys_info) {
    if (sysinfo(sys_info) != 0) {
        return;
    }
}

void get_current_time(char* buffer, size_t size) {
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

void get_load_average(double* load_avg) {
    if (getloadavg(load_avg, 3) == -1) {
        return;
    }
}

void get_memory_usage(struct sysinfo* sys_info, unsigned long* used_ram, unsigned long* total_ram) {
    *total_ram = sys_info->totalram / 1024;
    *used_ram = (*total_ram) - (sys_info->freeram / 1024);
}

void get_swap_usage(struct sysinfo* sys_info, unsigned long* used_swap, unsigned long* total_swap) {
    *total_swap = sys_info->totalswap / 1024;
    *used_swap = (*total_swap) - (sys_info->freeswap / 1024);
}

void get_summary(WINDOW* top_win, CpuTimes* prev_cpu_times, SummaryData* s) {
    get_system_info(&s->sys_info);

    get_current_time(s->current_time, sizeof(s->current_time));

    get_load_average(s->load_avg);

    get_memory_usage(&s->sys_info, &s->used_ram, &s->total_ram);

    get_swap_usage(&s->sys_info, &s->used_swap, &s->total_swap);

    CpuTimes curr_cpu_times;
    get_cpu_times(&curr_cpu_times);
    s->total_cpu_usage = calculate_cpu_usage(prev_cpu_times, &curr_cpu_times);
    
    *prev_cpu_times = curr_cpu_times;
    
}

void display_summary_top(WINDOW* top_win, SummaryData s)
{
    box(top_win, 0, 0);
    mvwprintw(top_win, 1, 1, "Uptime: %ld days, %ld:%02ld:%02ld", 
              s.sys_info.uptime / 86400, (s.sys_info.uptime % 86400) / 3600, 
              (s.sys_info.uptime % 3600) / 60, s.sys_info.uptime % 60);
    mvwprintw(top_win, 2, 1, "Current time: %s", s.current_time);
    mvwprintw(top_win, 3, 1, "Load average: %.2f, %.2f, %.2f", 
              s.load_avg[0], s.load_avg[1], s.load_avg[2]);
    mvwprintw(top_win, 4, 1, "Total tasks: %d", nr_processes);
    mvwprintw(top_win, 5, 1, "Total CPU usage: %.2f%%", s.total_cpu_usage);
    mvwprintw(top_win, 6, 1, "Memory usage: %lu KB / %lu KB", 
              s.used_ram, s.total_ram);
    mvwprintw(top_win, 7, 1, "Swap usage: %lu KB / %lu KB", 
              s.used_swap, s.total_swap);
}

void init_window(WINDOW** top_win, WINDOW** proc_win, WINDOW** options_win) {
    *top_win = newwin(HEIGHT_TOP, WIDTH, 0, 0);
    box(*top_win, 0, 0);
    wrefresh(*top_win);


    *proc_win = newwin(HEIGHT_PROC, WIDTH, HEIGHT_TOP, 0);
    box(*proc_win, 0, 0);
    wrefresh(*proc_win);

    *options_win = newwin(HEIGHT_OPT, WIDTH, HEIGHT_TOP + HEIGHT_PROC, 0);

    refresh();
}

void show_tutorial_commands(WINDOW* win, int line)
{
    mvwprintw(win, line, 0, "F1 Help     F2 Sort By     F3 Nice-     F4 Nice+     F5 Kill");
}

void show_sorting_options(WINDOW* win, int line)
{
    mvwprintw(win, line, 0, "F1 Pid     F2 PPid     F3 User     F4 Nice     Esc Back");
}

void display_help()
{
    mvprintw(1, 2, "F2 - sort by a criteria");
    mvprintw(2, 2, "F1, F2, F3, F4 Sort by mentioned criteria");
    mvprintw(3, 2, "If a button is pressed 2 times it will sort in descending order");
    mvprintw(4, 2, "If it is pressed 3 times it will make the list unsorted");
    mvprintw(5, 2, "F3 - Make nice value decrease by 1 (must be run using sudo)");
    mvprintw(6, 2, "F4 - Make nice value increase by 1 (sudo may be needed)");
    mvprintw(7, 2, "F5 - Kill selected process (sudo may be needed)");
    mvprintw(8, 2, "Commands that request sudo will not be run unless sudo");
    mvprintw(8, 2, "Right Key - Extend menu, also show ARGS");
    mvprintw(9, 2, "Left Key - Retract menu");
    mvprintw(11, 2, "Press F1 to go back");
    mvprintw(12, 2, "Press q to quit");
}

void terminate_process(pid_t pid) {
    char command[50];
    snprintf(command, sizeof(command), "kill -9 %d", pid);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        return;
    }

    pclose(fp);
}

void change_nice_value(pid_t pid, int increment) {
    char command[100];
    snprintf(command, sizeof(command), "ps -o ni= -p %d", pid);
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        return;
    }

    int current_nice;
    if (fscanf(fp, "%d", &current_nice) != 1) {
        pclose(fp); 
        return;
    }
    pclose(fp); 

    int new_nice = current_nice + increment;

    snprintf(command, sizeof(command), "renice %d -p %d", new_nice, pid);
    fp = popen(command, "r");
    if (fp == NULL) {
        return;
    }
    pclose(fp); 
}

void display_search(WINDOW* win, char * buffer, int line)
{
    mvwprintw(win, line, 0, "Search: ");
    mvwprintw(win, line, strlen("Search: "), "%s", buffer);
}

void resize_windows(WINDOW* top_win, WINDOW* proc_win, WINDOW* options_win)
{
    wresize(top_win, HEIGHT_TOP, WIDTH);
    wresize(proc_win, HEIGHT_PROC, WIDTH);
    wresize(options_win, HEIGHT_OPT, WIDTH);
    mvwin(proc_win, HEIGHT_TOP, 0);
    mvwin(options_win, HEIGHT_TOP + HEIGHT_PROC, 0);
}

void clear_refresh_windows(WINDOW* top_win, WINDOW* proc_win, WINDOW* options_win)
{
    clear();
    refresh();
    wclear(options_win);
    wclear(proc_win);
    wclear(top_win);
    wrefresh(options_win);
    wrefresh(proc_win);
    wrefresh(top_win);
}

void monitor_processes() {
    WINDOW* top_win, * proc_win, *options_win;
    init_window(&top_win, &proc_win, &options_win);
    int height, width;
    int right_pressed=0;
    int selected_pid;
    int ch;
    char search_buffer[50]="";
    int index_sbuffer=0;
    int start_index = 0;
    int highlight = 0;
    sorting_state ss = None;
    Process processes[MAX_PROCESSES];
    time_t current_time;
    time_t timer_time = time(0);
    CpuTimes prev_cpu_times;
    SummaryData sumData;
    int current_option=0;
    int display_count;
    get_processes(processes);
    get_summary(top_win, &prev_cpu_times, &sumData);

    while (1) {
        getmaxyx(stdscr, height, width);
        if (height < (HEIGHT_PROC + HEIGHT_TOP + HEIGHT_OPT) || width < WIDTH) {
            print_terminal_too_small(HEIGHT_PROC + HEIGHT_TOP + HEIGHT_OPT, WIDTH);
            refresh();
            getch();
            clear();
            continue;
        }
        current_time = time(0);
        if (difftime(current_time, timer_time) >= SECONDS) {
            timer_time = current_time;
            get_processes(processes);
            get_summary(top_win, &prev_cpu_times, &sumData);
        }

        resize_windows(top_win, proc_win, options_win); //Resize deoarece in cazul in care terminalul nu era la dimensiunea corecta aparea un bug ciudat

        clear_refresh_windows(top_win, proc_win, options_win);

        if(current_option == 2)
        {
            display_help();
        }
        else
        {
            display_processes_info(proc_win, start_index, highlight, processes, ss, search_buffer, &display_count, &selected_pid, right_pressed);
            display_summary_top(top_win, sumData);
            display_button_to_quit(options_win, 2);
            display_search(options_win, search_buffer, 1);

            switch (current_option)
            {
            case 0:
                show_tutorial_commands(options_win, 0);
                break;
            case 1:
                show_sorting_options(options_win, 0);
                break;
            default:
                break;
            }
        }
        
        wrefresh(proc_win);
        wrefresh(options_win);
        wrefresh(top_win);
        refresh();

        ch = getch();

        switch (ch) {
        case KEY_DOWN:
            if (highlight < HEIGHT_PROC - 2 && highlight + start_index < nr_processes - 1 && highlight < display_count - 1) {
                highlight++;
            }
            else if (start_index < nr_processes - 1 - HEIGHT_PROC) {
                start_index++;
            }
            break;
        case KEY_UP:
            if (highlight > 0) {
                highlight--;
            }
            else if (start_index > 0) {
                start_index--;
            }
            break;
        case KEY_RIGHT:
            right_pressed = 1;
        break;
        case KEY_LEFT:
            right_pressed = 0;
        break;
        case 'q':
            return;
        case KEY_RESIZE:
            getmaxyx(stdscr, height, width);
            refresh();
            if (height < (HEIGHT_PROC + HEIGHT_TOP + 2) || width < WIDTH) { continue; }
            break;
        case KEY_F(1):
        if (current_option == 1)
        {
            if(ss == Pid)
            {
                ss = Pid_desc;
                qsort(processes, nr_processes, sizeof(Process), compare_by_pid_desc);
            }
            else if (ss == Pid_desc)
            {
                ss = None;
            }
            else
            {
                ss = Pid;
                qsort(processes, nr_processes, sizeof(Process), compare_by_pid);
            }
        }
        else if (current_option == 0)
        {
            current_option = 2;
            display_help();
        }
        else if (current_option == 2)
        {
            current_option = 0;
        }
        break;
        case KEY_F(2):
            if(current_option == 0)
            {
                current_option = 1;
            }
            else if (current_option == 1)
            {
                if(ss == PPid)
                {
                    ss = PPid_desc;
                    qsort(processes, nr_processes, sizeof(Process), compare_by_ppid_desc);
                }
                else if (ss == PPid_desc)
                {
                    ss = None;
                }
                else
                {
                    ss = PPid;
                    qsort(processes, nr_processes, sizeof(Process), compare_by_ppid);
                }
                
            }
            break;
        case KEY_F(3):
        if (current_option == 1)
        {
            if(ss == User)
            {
                ss = User_desc;
                qsort(processes, nr_processes, sizeof(Process), compare_by_user_desc);
            }
            else if (ss == User_desc)
            {
                ss = None;
            }
            else
            {
                ss = User;
                qsort(processes, nr_processes, sizeof(Process), compare_by_user);
            }
        }
        else if (current_option == 0)
        {
            change_nice_value(selected_pid, -1); // Decrease nice
        }
        break;
        case KEY_F(4):
        if (current_option == 1)
        {
            if(ss == Nice)
            {
                ss = Nice_desc;
                qsort(processes, nr_processes, sizeof(Process), compare_by_nice_desc);
            }
            else if (ss == Nice_desc)
            {
                ss = None;
            }
            else
            {
                ss = Nice;
                qsort(processes, nr_processes, sizeof(Process), compare_by_nice);
            }
        }
        else if (current_option == 0)
        {
            change_nice_value(selected_pid, 1); // Increase nice
        }
        break;
        case KEY_F(5):
        if (current_option == 1)
        {
            ;
        }
        else if (current_option == 0)
        {
            terminate_process(selected_pid);
        }
        break;
        case 27: // esc sau alt 
            if (current_option == 1)
            {
                current_option = 0;
            }
            break;
        case KEY_BACKSPACE:
            if(index_sbuffer > 0)
            {
                search_buffer[--index_sbuffer] = '\0';
            }
            break;
        default:
            if(index_sbuffer < 49)
            {
                highlight = 0;
                search_buffer[index_sbuffer++] = ch;
                search_buffer[index_sbuffer] = '\0';
            }
        break;   
        }
    }
}