#include "menuHandler.h"
#include "showSpecs.h"
#include "processHandler.h"
#include "utils.h"


void display_menu(int highlight) {
    clear();
    char *options[NUM_OPTIONS] = {"Show Specs", "Monitor Processes", "Option 3", "Option 4"};
    for (int i = 0; i < NUM_OPTIONS; ++i) {
        if (i == highlight)
            attron(A_REVERSE);
        mvprintw(i, 0, "%s", options[i]);
        if (i == highlight)
            attroff(A_REVERSE);
    }
    mvprintw(NUM_OPTIONS, 0, "Press 'q' to exit.");
    refresh();
}

void handle_menu()
{
    int highlight = 0;
    int choice = 0;
    int c;

    while (1) {
        display_menu(highlight);
        c = getch();
        switch (c) {
            case KEY_UP:
                if (highlight == 0)
                    highlight = NUM_OPTIONS - 1;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == NUM_OPTIONS - 1)
                    highlight = 0;
                else
                    ++highlight;
                break;
            case 10: // Enter
                choice = highlight;
                if (choice == 0) {
                    clear();
                    show_specs();
                } else if (choice == 1) {
                    clear();
                    monitor_processes();
                } else if (choice == 2 || choice == 3) {
                    clear();
                    printw("Option %d is currently empty for further development.\n", choice + 1);
                    getch();
                }
                break;
            case 'q': // Exit on 'q'
                return;
            default:
                break;
        }
    }
    
}