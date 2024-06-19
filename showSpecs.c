#include "showSpecs.h"
#include "utils.h"
#define HEIGHT 30
#define MENU_WIDTH 15
#define WIDTH 100    
#define MENU_OPTIONS 3

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
    char *menu[MENU_OPTIONS] = {"Summary", "CPU", "MEMORY"};
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

void display_details(WINDOW *detail_win, int highlight) {
    werase(detail_win);

    switch (highlight) {
        case 0: // Summary
            display_summary(detail_win);
            break;
        case 1: // CPU
            mvwprintw(detail_win, 1, 2, "CPU details: Processor information");
            break;
        case 2: // MEMORY
            mvwprintw(detail_win, 1, 2, "Memory details: RAM usage");
            break;
    }
    box(detail_win, 0, 0);
    wrefresh(detail_win);
}

void show_specs() {
        WINDOW *menu_win, *detail_win;
    int highlight = 0;
    int choice;
    int ch;

     // Initialize windows
    init_windows(&menu_win, &detail_win);

    // Main loop
    while (1) {
        display_menu_specs(menu_win, highlight);
        display_details(detail_win, highlight);
        display_button_to_quit(HEIGHT + 1);

        ch = wgetch(menu_win); // Get user input from menu window

        switch (ch) {
            case '\t': // TAB key to switch between windows
                highlight = (highlight + 1) % MENU_OPTIONS;
                break;
            case 'q': // 'q' to quit
                return;
        }
    }
}