#include <stdio.h>
#include "pico/stdlib.h"
#include "flash_lib.h"

int main() {
    stdio_init_all();

    sleep_ms(2000);
    printf("Starting program...\n");

    flash_lib_example();
    
    printf("Finished...\n");
    while (true) {
        sleep_ms(1000);
    }
}
