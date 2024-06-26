#include "showSpecs.h"
#include "utils.h"
#include <string.h>
#define HEIGHT 40
#define MENU_WIDTH 15
#define WIDTH 100    
#define MENU_OPTIONS 5

typedef struct 
{
    char peripherals_data[3000];
    char storage_data[1024];
    char memory_data[1024];
}data;

void init_windows(WINDOW **menu_win, WINDOW **detail_win) {
    // Create menu window
    *menu_win = newwin(HEIGHT, MENU_WIDTH, 0, 0);
    box(*menu_win, 0, 0); // Add a border
    refresh();
    wrefresh(*menu_win);

    // Create detail window
    *detail_win = newwin(HEIGHT, WIDTH, 0, MENU_WIDTH);
    box(*detail_win, 0, 0); // Add a border
    refresh();
    wrefresh(*detail_win);
}

void display_menu_specs(WINDOW *menu_win, int highlight) {
    char *menu[MENU_OPTIONS] = {"Summary", "CPU", "Memory", "Storage", "Peripherals"};
    int x, y;

    // Clear and refresh the menu window
    werase(menu_win);
    box(menu_win, 0, 0);
    wrefresh(menu_win);

    // Print menu options
    for (y = 1; y <= MENU_OPTIONS; ++y) {
        x = 2;
        if (highlight == y - 1) {
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", menu[y - 1]);
            wattroff(menu_win, A_REVERSE);
        } else {
            mvwprintw(menu_win, y, x, "%s", menu[y - 1]);
        }
    }
    wrefresh(menu_win);
}

void display_summary(WINDOW *detail_win)
{
    char buffer[1024];
    int y=1;
    mvwprintw(detail_win, y, 2, "Model of CPU: ");
    y++;
    FILE* fp;
    fp = popen("grep -m 1 'model name' /proc/cpuinfo | tr -s \" \" | cut -f3- -d\" \"", "r");
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        mvwprintw(detail_win, y++, 4, "%s", buffer);
    }
    pclose(fp);
    y++;
    mvwprintw(detail_win, y, 2, "Memory RAM: ");
    y++;
    fp = popen("free -h --si | tr -s \" \" | cut -f1-2 -d\" \" | tail -2", "r");
    if (handleErrorCommand(fp, detail_win))
        return;
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        mvwprintw(detail_win, y++, 4, "%s", buffer);
    }
    pclose(fp);
    y++;

    mvwprintw(detail_win, y, 2, "Graphics Card(s): ");
    y++;
    fp = popen("lspci | egrep \"VGA|3D\" | cut -f2- -d\" \" | grep -Eo \": .+\" | cut -f2- -d' '", "r");
    if (handleErrorCommand(fp, detail_win))
        return;

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        mvwprintw(detail_win, y++, 4, "%s", buffer);
    }
    pclose(fp);
    y++;
    mvwprintw(detail_win, y, 2, "Storage: ");
    y++;
    fp = popen("lsblk -o NAME,SIZE,MODEL", "r");
    if (handleErrorCommand(fp, detail_win))
        return;
    fgets(buffer, sizeof(buffer), fp);
    mvwprintw(detail_win, y++, 4, "%s", buffer);
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (buffer[0] == 's')
            mvwprintw(detail_win, y++, 4, "%s", buffer);
    }
    pclose(fp);
    y++;
    mvwprintw(detail_win, y, 2, "Audio: ");
    y++;
    fp = popen("lspci | grep -i audio | grep -Eo \": .+\" | cut -f2- -d\" \"", "r");
    if (handleErrorCommand(fp, detail_win))
        return;
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        mvwprintw(detail_win, y++, 4, "%s", buffer);
    }
    pclose(fp);

}

void display_CPU(WINDOW *detail_win)
{
    FILE* fp;
    int y = 1;
    char buffer[1024];
    fp=popen("lscpu | head -16", "r");
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        mvwprintw(detail_win, y++, 2, "%s", buffer);
        y+= (int)(strlen(buffer) / WIDTH);
    }

    pclose(fp);
    mvwprintw(detail_win, y++, 2, "%s", "Cache Memory:");
    fp=popen("lscpu -C", "r");
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        mvwprintw(detail_win, y++, 2, "%s", buffer);
    }
    
    pclose(fp);
}

void display_Memory(WINDOW *detail_win, char buffer[1024], int start_index)
{

    int y = 1;
    char *p = strtok(strdup(buffer), "\n");
    for (int i = 0; i < start_index && p; i++)
    {
        p = strtok(NULL, "\n");
    }
    while (p) {
        mvwprintw(detail_win, y++, 2, "%s", p);
        y+= (int)(strlen(p) / WIDTH);
        p=strtok(NULL, "\n");
    }

}

void display_storage(WINDOW *detail_win, char buffer[1024], int start_index)
{
    

    int y = 1;
    char *p = strtok(strdup(buffer), "\n");
    for (int i = 0; i < start_index && p; i++)
    {
        p = strtok(NULL, "\n");
    }
    while (p) {
        mvwprintw(detail_win, y++, 2, "%s", p);
        y+= (int)(strlen(p) / WIDTH);
        p=strtok(NULL, "\n");
    }

}

void display_peripherals(WINDOW *detail_win, int start_index, char buffer[3000])
{
    int y = 1;
    int current_index=-1;
    char* p = strtok(strdup(buffer), "\n");

    for (int i = 0; i < start_index && p; i++)
    {
        p = strtok(NULL, "\n");
    }

    while (p) {
        current_index++;
        if (y >= HEIGHT )
        {
            break;
        }
            mvwprintw(detail_win, y++, 2, "%s", p);
            y+= (int)(strlen(p) / WIDTH);
        p=strtok(NULL, "\n");
    }
    while(p)
    {
        p = strtok(NULL, "\n");
    }
    
}

void display_details(WINDOW *detail_win, int highlight, int start_index, data dataHW) {
    werase(detail_win);

    switch (highlight) {
        case 0: // Summary
            display_summary(detail_win);
            break;
        case 1: // CPU
            display_CPU(detail_win);
            break;
        case 2: // MEMORY
            display_Memory(detail_win, dataHW.memory_data, start_index);
            break;
        case 3: // Storage
            display_storage(detail_win, dataHW.storage_data, start_index);
            break;
        case 4: // Peripherals
        display_peripherals(detail_win, start_index, dataHW.peripherals_data);
            break;
    }
    box(detail_win, 0, 0);

    wrefresh(detail_win);
}

void init_data(data* dataHW)
{
    clear();
    printw("%s", "Gathering data...");
    refresh();
    FILE* fp;
    fp=popen("lshw -C input 2> /dev/null", "r");
    char line[200];
    strcpy(dataHW->peripherals_data, "");
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        strcat(dataHW->peripherals_data, line);
    }
    pclose(fp);

    fp=popen("lshw -C storage 2> /dev/null", "r");
    strcpy(dataHW->storage_data, "");
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        strcat(dataHW->storage_data, line);
    }
    pclose(fp);

    fp=popen("lshw -C memory 2> /dev/null", "r");
    strcpy(dataHW->memory_data, "");
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        strcat(dataHW->memory_data, line);
    }
    pclose(fp);
}

void display_tutorial(int y)
{
    mvprintw(y, 0, "Press TAB to shuffle through menus");
}

void _resize_windows(WINDOW* menu_win, WINDOW* specs_win)
{
    wresize(menu_win, HEIGHT, MENU_WIDTH);
    wresize(specs_win, HEIGHT, WIDTH);
    mvwin(menu_win, 0, 0);
    mvwin(specs_win, 0, MENU_WIDTH);
}

void show_specs() {
        WINDOW *menu_win, *detail_win;
    int highlight = 0;
    int start_index=0;
    int height, width;
    int ch;
    data dataHW;

     // Initialize windows
    init_windows(&menu_win, &detail_win);
    init_data(&dataHW);
    // Main loop
    while (1) {
        getmaxyx(stdscr, height, width);
        if (height < (HEIGHT + 3) || width < WIDTH + MENU_WIDTH) {
            print_terminal_too_small(HEIGHT + 3, WIDTH + MENU_WIDTH);
            refresh();
            getch();
            clear();
            continue;
        }
        _resize_windows(menu_win, detail_win);
        display_menu_specs(menu_win, highlight);
        display_details(detail_win, highlight, start_index, dataHW);
        display_button_to_quit(stdscr ,HEIGHT + 1);
        display_tutorial(HEIGHT+2);
        refresh();

no_important_key_pressed:
        ch = getch(); // Get user input from menu window

        switch (ch) {
            case '\t': // TAB key to switch between windows
                start_index=0;
                highlight = (highlight + 1) % MENU_OPTIONS;
                break;
            case KEY_DOWN:
                start_index++;
                break;
            case KEY_UP:
                if(start_index > 0)
                    start_index--;
                break;
            case 'q': // 'q' to quit
                return;
            case KEY_RESIZE:
                getmaxyx(stdscr, height, width);
                refresh();
                if (height < (HEIGHT + 3) || width < WIDTH + MENU_WIDTH) { continue; }
                break;
            default:
                goto no_important_key_pressed;
        }
    }
}