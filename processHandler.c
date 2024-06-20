#include "utils.h"
#include "processHandler.h"
#include <ncurses.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define WIDTH 80
#define HEIGHT_TOP 10
#define HEIGHT_PROC 30

int nr_processes=0;

typedef struct 
{
    int pid;
    int ppid;
    char user[BUFFER_SIZE];
    char state;
    char name[BUFFER_SIZE];
}Process;


int is_number(char buffer[BUFFER_SIZE/4])
{
    for(int i = 0; i < strlen(buffer); i++)
    {
        if (!isdigit(buffer[i]))
            return 0;
    }
    return 1;
}

char* get_username(int uid)
{
    char command[256] = "grep -E \":x:";
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "%d", uid);
    strcat(command, buffer);
    strcat(command, "\" /etc/passwd | cut -f1 -d:");
    FILE* fp = popen(command, "r");
    fgets(buffer, sizeof(buffer), fp);
    pclose(fp);
    if (buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = '\0';
    return strdup(buffer);
}

void get_processes_info(WINDOW* proc_win, int start_index, int highlight) {
    
    FILE* lsCommand;
    lsCommand = popen("ls /proc", "r");
    char procID[BUFFER_SIZE/4];
    char buffer[BUFFER_SIZE];
    Process processes[HEIGHT_PROC];
    int count=0;
    int current_process=-1;
    nr_processes=0;
    mvwprintw(proc_win, count+1, 2, "S\tOwner\tPID\tPPID\tCMD");
    count++;
    while (fgets(procID, sizeof(procID), lsCommand) != NULL)
    {
        nr_processes++;

        if(nr_processes <= start_index)
            continue;
        if (count >= HEIGHT_PROC - 2)
            continue;
        if(procID[strlen(procID) - 1] == '\n')
            procID[strlen(procID) - 1] = '\0';
        if(!is_number(procID))
            {continue;}


        char filename[BUFFER_SIZE];
        strcpy(filename, "");
        strcat(filename, "/proc/");
        strcat(filename, procID);
        strcat(filename, "/status");
        FILE* f = fopen(filename, "r");

        if(!f)
            continue;
        
        while(fgets(buffer, sizeof(buffer), f) != NULL)
        {
            processes[count].pid = atoi(procID);
            
            if(strncmp(buffer, "Name:", 5) == 0)
            {
                sscanf(buffer, "Name:\t%s", processes[count].name);
            }
            else if (strncmp(buffer, "State:", 6) == 0)
            {
                sscanf(buffer, "State:\t%c", &processes[count].state);
            }
            else if (strncmp(buffer, "PPid:", 5) == 0)
            {
                sscanf(buffer, "PPid:\t%d", &processes[count].ppid);
            }
            else if (strncmp(buffer, "Uid:", 4) == 0)
            {
                int uid;
                sscanf(buffer, "Uid:\t%d", &uid);
                char* username= get_username(uid);
                strcpy(processes[count].user, username);
                free(username);
            }
        }
        fclose(f);
        if (count == highlight)
            wattron(proc_win, A_REVERSE);
        mvwprintw(proc_win, count + 1, 2, "%c\t%s\t%d\t%d\t%s", processes[count].state, processes[count].user, processes[count].pid, processes[count].ppid, processes[count].name);
        if (count == highlight)
            wattroff(proc_win, A_REVERSE);
        count++;
    }
    box(proc_win, 0, 0); 
    wrefresh(proc_win);

    pclose(lsCommand);

}

void get_summary(WINDOW* top_win)
{
    box(top_win, 0, 0); 
    wrefresh(top_win);
}

void init_window(WINDOW **top_win, WINDOW **proc_win) {
    
    *top_win = newwin(HEIGHT_TOP, WIDTH, 0, 0);
    box(*top_win, 0, 0); 
    wrefresh(*top_win);

    
    *proc_win = newwin(HEIGHT_PROC, WIDTH, HEIGHT_TOP, 0);
    box(*proc_win, 0, 0); 
    wrefresh(*proc_win);
    refresh();
}

void monitor_processes() {
    WINDOW *top_win, *proc_win;
    init_window(&top_win, &proc_win);
    int height, width;
    int ch;
    int start_index=0;
    int highlight=2;
    wclear(proc_win);
    while(1)
    {
        getmaxyx(stdscr, height, width);
        if (height < (HEIGHT_PROC + HEIGHT_TOP + 1) || width < WIDTH)
        {
            print_terminal_too_small(HEIGHT_PROC + HEIGHT_TOP + 2, WIDTH);
            refresh();
            getch();
            clear();
            continue;
        }
        wclear(proc_win);
        get_summary(top_win);
        get_processes_info(proc_win, start_index, highlight);
        display_button_to_quit(HEIGHT_PROC + HEIGHT_TOP + 2);
        refresh();
no_useful_key_pressed:
        ch = getch();

        switch (ch) {
            case KEY_DOWN:
            if(highlight < HEIGHT_PROC - 3)
                {
                    highlight++;
                }
            else if(start_index < nr_processes - 1)
                start_index++;
                break;
            case KEY_UP:
                if(highlight > 1)
                    highlight--;
                break;
            case 'q': // 'q' to quit
                return;
            case KEY_RESIZE:
                //delwin(proc_win);
                //delwin(top_win);
                //init_window(&top_win, &proc_win);
                getmaxyx(stdscr, height, width);
                refresh();
            if (height < (HEIGHT_PROC + HEIGHT_TOP + 2) || width < WIDTH)
                {continue;}
            else
                {
                    goto no_useful_key_pressed;
                }
                break;
            default:
                goto no_useful_key_pressed;
        }
        
    }
}
