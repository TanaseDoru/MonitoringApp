#include "utils.h"
#include <ncurses.h>
void display_scrollable_output(const char *command) {
    FILE *fp;
    char buffer[256];
    int row, col;
    int ch;
    int y = 0;
    int max_y = 0;

    WINDOW *win = newwin(LINES - 1, COLS, 0, 0);
    keypad(win, TRUE);

    fp = popen(command, "r");
    if (fp == NULL) {
        mvprintw(0, 0, "Failed to run command\n");
        getch();
        return;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        mvwprintw(win, y++, 0, "%s", buffer);
    }
    pclose(fp);

    max_y = y;

    wrefresh(win);
    y = 0;

    while ((ch = wgetch(win)) != 'q') {
        switch (ch) {
            case KEY_UP:
                if (y > 0)
                    y--;
                break;
            case KEY_DOWN:
                if (y + LINES - 1 < max_y)
                    y++;
                break;
        }
        wscrl(win, y);
        wrefresh(win);
    }

    delwin(win);
}
