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

#define WIDTH 80
#define HEIGHT_TOP 10
#define HEIGHT_PROC 30
#define MAX_PROCESSES 300
#define MAX_THREADS 4
#define BUFFER_SIZE 1024
#define HEIGHT_OPT 8

int nr_processes = 0;

typedef struct {
    int pid;
    int ppid;
    char user[BUFFER_SIZE];
    char state;
    char name[BUFFER_SIZE];
    int nice;
    float cpu_usage;
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
    Pid,
    PPid,
    User,
    State
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

int compare_by_state(const void* a, const void* b) {
    return ((Process*)a)->state - ((Process*)b)->state;
}

int compare_by_name(const void* a, const void* b) {
    return strcmp(((Process*)a)->name, ((Process*)b)->name);
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


    snprintf(filename, sizeof(filename), "/proc/%s/stat", pid);
    f = fopen(filename, "r");
    if (!f) return;

    unsigned long utime, stime, starttime;
    long nice;

     // Variablie temporale ca dadea warning pentru *
    int temp_int;
    unsigned int temp_uint;
    long temp_long;
    unsigned long temp_ulong;
    char temp_char;
    char* temp_pchar;

    if (fgets(buffer, sizeof(buffer), f) != NULL) {
        sscanf(buffer,
               "%d %s %c %d %d %d %d %d %u %u %u %u %u %lu %lu %ld %ld %ld %ld %ld %lu",
               &temp_int, temp_pchar, &temp_char, &temp_int, &temp_int, &temp_int, &temp_int,
               &temp_int, &temp_uint, &temp_uint, &temp_uint, &temp_uint, &temp_uint, 
               &utime, &stime, &nice, &temp_long, &temp_long, &temp_long, &temp_long, 
               &starttime);
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
                current_proc->cpu_usage = 100 * (total_time / seconds);
            } else {
                current_proc->cpu_usage = 0.0;
            }
        }
    }
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

void display_processes_info(WINDOW* proc_win, int start_index, int highlight, Process* proc, sorting_state ss) {
    
    switch (ss)
    {
        case Pid:
            qsort(proc, nr_processes, sizeof(Process), compare_by_pid);
        break;
        case PPid:
            qsort(proc, nr_processes, sizeof(Process), compare_by_ppid);
        break;
        case State:
            qsort(proc, nr_processes, sizeof(Process), compare_by_state);
        break;
        case User:
            qsort(proc, nr_processes, sizeof(Process), compare_by_user);
        break;  
        default:
        break;
    }
    
    char buffer[BUFFER_SIZE];
    mvwprintw(proc_win, 1, 2, "S\tOwner\tPID\tPPID\tNice\tCPU\tCMD");
    int current_process = start_index;
    for (int i = 0; i < HEIGHT_PROC - 3; i++) {
        if (current_process >= nr_processes) break;
        if (i == highlight)
            wattron(proc_win, A_REVERSE);
        mvwprintw(proc_win, i + 2, 2, "%c\t%s\t%d\t%d\t%d\t%.1f\t%s", proc[current_process].state, 
        proc[current_process].user, proc[current_process].pid, proc[current_process].ppid, proc[current_process].nice, proc[current_process].cpu_usage,
        proc[current_process].name);
        if (i == highlight)
            wattroff(proc_win, A_REVERSE);
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
    fgets(buffer, sizeof(buffer), fp) == NULL;
    fclose(fp);
    sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu\n",
        &times->user, &times->nice, &times->system, &times->idle,
        &times->iowait, &times->irq, &times->softirq, &times->steal);
        
    
}

void get_system_info(struct sysinfo* sys_info) {
    if (sysinfo(sys_info) != 0) {
        perror("sysinfo");
        exit(EXIT_FAILURE);
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
        perror("getloadavg");
        exit(EXIT_FAILURE);
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
    mvwprintw(top_win, 4, 1, "Total CPU usage: %.2f%%", s.total_cpu_usage);
    mvwprintw(top_win, 5, 1, "Memory usage: %lu KB / %lu KB", 
              s.used_ram, s.total_ram);
    mvwprintw(top_win, 6, 1, "Swap usage: %lu KB / %lu KB", 
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

void change_nice_value(int pid, int inc)
{
    int priority = getpriority(PRIO_PROCESS, pid);
    setpriority(PRIO_PROCESS, pid, priority + inc);
}

void show_tutorial_commands(WINDOW* win, int line)
{
    mvwprintw(win, line, 0, "F1 Help     F2 Sort By     F3 Nice-     F4 Nice+     F5 Kill");
}

void show_sorting_options(WINDOW* win, int line)
{
    mvwprintw(win, line, 0, "F1 Pid     F2 PPid     F3 User     F4 State     Esc Back");
}

int terminate_process(pid_t pid) {
    if (kill(pid, SIGKILL) == 0) {
        return 0; 
    } else {
        return 1;
    }
}

void display_search(WINDOW* win, char * buffer, int line)
{
    mvwprintw(win, line, 0, "Search: ");
    mvwprintw(win, line, strlen("Search: "), "%s", buffer);
}

void monitor_processes() {
    WINDOW* top_win, * proc_win, *options_win;
    init_window(&top_win, &proc_win, &options_win);
    int height, width;
    int ch;
    char search_buffer[256]="";
    int index_sbuffer=0;
    int start_index = 0;
    int highlight = 0;
    sorting_state ss = None;
    Process processes[MAX_PROCESSES];
    time_t current_time;
    time_t timer_time = time(0);
    CpuTimes prev_cpu_times;
    SummaryData sumData;
    wclear(proc_win);
    get_processes(processes);
    get_summary(top_win, &prev_cpu_times, &sumData);
    int current_option=0;

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
        if (difftime(current_time, timer_time) >= 2.5) {
            timer_time = current_time;
            get_processes(processes);
            get_summary(top_win, &prev_cpu_times, &sumData);
        }
        clear();
        refresh();
        wclear(options_win);
        wclear(proc_win);
        wclear(top_win);
        wrefresh(options_win);
        wrefresh(proc_win);
        wrefresh(top_win);
        display_processes_info(proc_win, start_index, highlight, processes, ss);
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
        
        wrefresh(proc_win);
        wrefresh(options_win);
        wrefresh(top_win);
        refresh();

        ch = getch();

        switch (ch) {
        case KEY_DOWN:
            if (highlight < HEIGHT_PROC - 4 && highlight + start_index < nr_processes - 1) {
                highlight++;
            }
            else if (start_index < nr_processes - 1) {
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
        case 'q': // 'q' to quit
            return;
        case KEY_RESIZE:
            getmaxyx(stdscr, height, width);
            refresh();
            if (height < (HEIGHT_PROC + HEIGHT_TOP + 2) || width < WIDTH) { continue; }
            break;
        case KEY_F(1):
        if (current_option == 1)
        {
            ss = Pid;
            qsort(processes, nr_processes, sizeof(Process), compare_by_pid);
            

        }
        break;
        case KEY_F(2):
            if(current_option == 0)
            {
                current_option = 1;
            }
            else if (current_option == 1)
            {
                ss =PPid;
                qsort(processes, nr_processes, sizeof(Process), compare_by_ppid);
            }
            break;
        case KEY_F(3):
        if (current_option == 1)
        {
            ss = User;
            qsort(processes, nr_processes, sizeof(Process), compare_by_user);
        }
        else if (current_option == 0)
        {
            if (highlight + start_index < nr_processes)
            change_nice_value(processes[highlight + start_index].pid, -1); // Decrease nice
        }
        break;
        case KEY_F(4):
        if (current_option == 1)
        {
            ss = State;
            qsort(processes, nr_processes, sizeof(Process), compare_by_state);
        }
        else if (current_option == 0)
        {
            if (highlight + start_index < nr_processes)
            change_nice_value(processes[highlight + start_index].pid, 1); // Decrease nice
        }
        break;
        case KEY_F(5):
        if (current_option == 1)
        {
            ;//qsort(processes, nr_processes, sizeof(Process), compare_by_pid);
        }
        else if (current_option == 0)
        {
            terminate_process(processes[start_index + highlight].pid);
        }
        break;
        case KEY_F(6):
        if (current_option == 1)
        {
            ;//qsort(processes, nr_processes, sizeof(Process), compare_by_pid);
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
            if(index_sbuffer < 255)
            {
                search_buffer[index_sbuffer++] = ch;
                search_buffer[index_sbuffer] = '\0';
            }
        break;   
        }
    }
}