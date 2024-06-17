import curses
import time
import psutil

# Function to retrieve CPU temperature
def get_cpu_temperature():
    temperatures = psutil.sensors_temperatures()
    if "coretemp" in temperatures:
        for entry in temperatures["coretemp"]:
            if entry.label.startswith("Package"):
                return entry.current
    elif "cpu-thermal" in temperatures:
        for entry in temperatures["cpu-thermal"]:
            return entry.current
    return None

# Function to display the menu
def display_menu(stdscr, selected_row):
    stdscr.clear()
    h, w = stdscr.getmaxyx()
    
    menu = ["Monitorizare Sistem", "Opțiune 1", "Opțiune 2", "Ieșire"]
    
    for idx, item in enumerate(menu):
        x = w // 2 - len(item) // 2
        y = h // 2 - len(menu) // 2 + idx
        if idx == selected_row:
            stdscr.attron(curses.color_pair(1))
            stdscr.addstr(y, x, item)
            stdscr.attroff(curses.color_pair(1))
        else:
            stdscr.addstr(y, x, item)
    
    stdscr.refresh()

# Main function
def main(stdscr):
    curses.curs_set(0)  # Ascunde cursorul
    stdscr.nodelay(1)  # Non-blocking input
    curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_WHITE)
    
    current_row = 0
    display_menu(stdscr, current_row)
    
    while True:
        key = stdscr.getch()
        
        if key == curses.KEY_UP and current_row > 0:
            current_row -= 1
            display_menu(stdscr, current_row)
        elif key == curses.KEY_DOWN and current_row < 3:
            current_row += 1
            display_menu(stdscr, current_row)
        elif key == curses.KEY_ENTER or key in [10, 13]:
            display_menu(stdscr, current_row)
            if current_row == 0:
                # Display system monitoring information
                while True:
                    stdscr.clear()
                    cpu_usage = psutil.cpu_percent(interval=1)
                    mem_info = psutil.virtual_memory()
                    disk_info = psutil.disk_usage('/')
                    cpu_temp = get_cpu_temperature()
                    
                    stdscr.addstr(0, 0, f"Utilizare CPU: {cpu_usage}%")
                    stdscr.addstr(1, 0, f"Temperatura CPU: {cpu_temp} °C" if cpu_temp is not None else "Temperatura CPU: N/A")
                    stdscr.addstr(2, 0, f"Memorie disponibilă: {mem_info.available / (1024 ** 2):.2f} MB")
                    stdscr.addstr(3, 0, f"Spațiu pe disc disponibil: {disk_info.free / (1024 ** 3):.2f} GB")
                    
                    stdscr.addstr(5, 0, "Apasă 'q' pentru a reveni la meniu.")
                    stdscr.refresh()
                    
                    key = stdscr.getch()
                    if key == ord('q'):
                        break
            elif current_row == 1:
                # Option 1 - Replace with your functionality
                pass
            elif current_row == 2:
                # Option 2 - Replace with your functionality
                pass
            elif current_row == 3:
                break

if __name__ == "__main__":
    curses.wrapper(main)

