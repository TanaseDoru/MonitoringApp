#ifndef _UTILS
#define _UTILS
#include <ncurses.h>
#define BUFFER_SIZE 1024

void display_scrollable_output(const char *command);
void display_button_to_quit(WINDOW* win, int line);
int handleErrorCommand(FILE* fp, WINDOW *detail_win);
void print_terminal_too_small(int target_height, int target_width);
#endif