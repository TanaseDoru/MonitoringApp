#include "utils.h"
#include "processHandler.h"

void monitor_processes() {
    display_scrollable_output("ps aux --sort=-%cpu");
}
