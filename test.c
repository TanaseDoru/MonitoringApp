#include <ncurses.h>
#include <unistd.h>

int main() {
    initscr();  // Initialize the window
    cbreak();   // Line buffering disabled, Pass on every key press
    noecho();   // Don't echo() while we do getch
    curs_set(FALSE); // Hide the cursor
    while(1)
    {// Get the initial size of the terminal
    clear();
    int height, width;
    getmaxyx(stdscr, height, width);

    // Print the dimensions
    mvprintw(0, 0, "Height: %d, Width: %d", height, width);
    refresh();
    
    // Wait for user input before exiting
    getch();}

    endwin(); // End curses mode
    return 0;
}
