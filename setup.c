#include "utils.h"
#include "setup.h"
#include <ncurses.h>
#include <stdlib.h>

int begin_setup()
{
    initscr();            // Start curses mode
    cbreak();             // Line buffering disabled, Pass on every key press
    noecho();             // Don't echo while we do getch
    keypad(stdscr, TRUE); // Enable function keys like F1, F2, arrow keys, etc.

    return 0;
}

