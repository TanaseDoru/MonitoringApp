#include <ncurses.h>
#include "menuHandler.h"
#include "setup.h"

int main() {
    
    begin_setup();

    handle_menu();
    endwin(); // End curses mode
    return 0;
}



