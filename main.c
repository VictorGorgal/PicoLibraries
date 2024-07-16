#include <stdio.h>
#include "pico/stdlib.h"
#include "flash_lib/flash_lib.h"

// #define PAGE_SIZE 256
// #define NVS_SIZE 4096
// #define FLASH_WRITE_START (2 * 1024 * 1024)
// #define FLASH_READ_START (FLASH_WRITE_START + XIP_BASE)

// void print_bits(uint32_t data, uint8_t dataSize) {
//     for (uint8_t i = 0; i < dataSize; i++) {
//         printf("%d", (data >> (dataSize-1-i)) & 1);
//     }
// }

// void print_page(uint8_t *addr, uint8_t max_index) {
//     printf("Flash: ");
//     for (uint8_t i = 0; i < max_index; i++) {
//         print_bits(addr[i], 8);
//         printf(", ");
//     }
//     print_bits(addr[max_index], 8);
//     printf("\n");
// }

// void force_erase_program_memory(uint32_t addr) {
//     printf("WARNING: This will erase the program memory!\n");
    
//     sleep_ms(5);
//     flash_range_erase(addr, 8 * NVS_SIZE);
//     printf("Program memory erased.\n");
// }

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
