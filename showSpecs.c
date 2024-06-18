#include "showSpecs.h"
#include "utils.h"

void show_specs() {
    display_scrollable_output("lshw -short");
}