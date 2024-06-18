#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "menuHandler.h"

int main() {
    initscr();            // Start curses mode
    cbreak();             // Line buffering disabled, Pass on every key press
    noecho();             // Don't echo while we do getch
    keypad(stdscr, TRUE); // Enable function keys like F1, F2, arrow keys, etc.

    handle_menu();
    endwin(); // End curses mode
    return 0;
}



