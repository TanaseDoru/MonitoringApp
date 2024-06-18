#include "showSpecs.h"
#include "utils.h"
#define HEIGHT 5
#define MENU_WIDTH 20
#define WIDTH 10
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

void display_details(WINDOW *detail_win, int highlight) {
    werase(detail_win);
    box(detail_win, 0, 0);

    // Print details based on selected menu option
    switch (highlight) {
        case 0: // Summary
            mvwprintw(detail_win, 1, 2, "Summary: System overview");
            break;
        case 1: // CPU
            mvwprintw(detail_win, 1, 2, "CPU details: Processor information");
            break;
        case 2: // MEMORY
            mvwprintw(detail_win, 1, 2, "Memory details: RAM usage");
            break;
    }

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
        display_button_to_quit(MENU_OPTIONS + 2);

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