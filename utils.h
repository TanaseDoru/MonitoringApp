#ifndef _UTILS
#define _UTILS
#include <ncurses.h>

void display_scrollable_output(const char *command);
void display_button_to_quit(int line);
int handleErrorCommand(FILE* fp, WINDOW *detail_win);
#endif