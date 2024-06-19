#include "utils.h"
#include "setup.h"
#include <ncurses.h>
#include <stdlib.h>
#include <locale.h>

int begin_setup()
{
    setlocale(LC_ALL, "");// For Unicode
    initscr();            // Start curses mode
    cbreak();             // Line buffering disabled, Pass on every key press
    noecho();             // Don't echo while we do getch
    keypad(stdscr, TRUE); // Enable function keys like F1, F2, arrow keys, etc.

    return 0;
}

