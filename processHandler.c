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
#include <pwd.h>
#include <pthread.h>

#define WIDTH 80
#define HEIGHT_TOP 10
#define HEIGHT_PROC 30
#define MAX_PROCESSES 300
#define MAX_THREADS 4
#define BUFFER_SIZE 1024

int nr_processes = 0;

typedef struct {
    int pid;
    int ppid;
    char user[BUFFER_SIZE];
    char state;
    char name[BUFFER_SIZE];
} Process;

typedef struct {
    Process* proc;
    char** pid_list;
    int start;
    int end;
} ThreadData;

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

void display_processes_info(WINDOW* proc_win, int start_index, int highlight, Process* proc) {
    char buffer[BUFFER_SIZE];
    mvwprintw(proc_win, 1, 2, "S\tOwner\tPID\tPPID\tCMD");
    int current_process = start_index;
    for (int i = 0; i < HEIGHT_PROC - 3; i++) {
        if (current_process >= nr_processes) break;
        if (i == highlight)
            wattron(proc_win, A_REVERSE);
        mvwprintw(proc_win, i + 2, 2, "%c\t%s\t%d\t%d\t%s", proc[current_process].state, proc[current_process].user, proc[current_process].pid, proc[current_process].ppid, proc[current_process].name);
        if (i == highlight)
            wattroff(proc_win, A_REVERSE);
        current_process++;
    }
    box(proc_win, 0, 0);
    wrefresh(proc_win);
}

void get_summary(WINDOW* top_win) {
    box(top_win, 0, 0);
    wrefresh(top_win);
}

void init_window(WINDOW** top_win, WINDOW** proc_win) {
    *top_win = newwin(HEIGHT_TOP, WIDTH, 0, 0);
    box(*top_win, 0, 0);
    wrefresh(*top_win);

    *proc_win = newwin(HEIGHT_PROC, WIDTH, HEIGHT_TOP, 0);
    box(*proc_win, 0, 0);
    wrefresh(*proc_win);
    refresh();
}

void monitor_processes() {
    WINDOW* top_win, * proc_win;
    init_window(&top_win, &proc_win);
    int height, width;
    int ch;
    int start_index = 0;
    int highlight = 0;
    Process processes[MAX_PROCESSES];
    time_t current_time;
    time_t timer_time = time(0);
    wclear(proc_win);
    get_processes(processes);
    while (1) {
        getmaxyx(stdscr, height, width);
        if (height < (HEIGHT_PROC + HEIGHT_TOP + 1) || width < WIDTH) {
            print_terminal_too_small(HEIGHT_PROC + HEIGHT_TOP + 2, WIDTH);
            refresh();
            getch();
            clear();
            continue;
        }
        current_time = time(0);
        if (difftime(current_time, timer_time) >= 2.0) {
            timer_time = current_time;
            get_processes(processes);
        }
        wclear(proc_win);
        get_summary(top_win);
        display_processes_info(proc_win, start_index, highlight, processes);
        display_button_to_quit(HEIGHT_PROC + HEIGHT_TOP + 2);
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
        }
    }
}