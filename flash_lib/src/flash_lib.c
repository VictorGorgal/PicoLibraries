#include <stdlib.h>
#include <stdio.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "flash_lib.h"
#include "../private/hashmap.h"

/**
 * Hash map containing the hardware sector number as the key and
 * the library sector indentifier as the value
*/
HashMap *sectors;

uint32_t _lower_bound;
uint32_t _upper_bound;
uint16_t _logical_sectors_count;
uint8_t _group_by;

/**
 * @brief Initializes the library
 * 
 * *** Use ***
 * The flash memory is devided into sectors, each containing 4kb each, while this
 * library uses 'logical sectors', each logical sector will have a number of sectors
 * defined by the group_by attribute.
 * For example, if group_by is 64, each logical sector will be 4kb * 64 = 256kb in size.
 * 
 * This library can handle up to 65535 logical sectors, needing around 340kb of ram to do so.
 * It is advised to use large logical sector sizes to decrease ram usage.
 * 
 * Grouping by 64, this library will only need around 25kb of ram to handle 1Gb of flash
 * (4096 logical sectors of 64 real sectors each)
 * 
 * The write function also implements a dynamic wear leveling algorithm, for more information
 * check out the write_sector() function
 * 
 * To use this library, first you will need to allocate some logical sectors, The alloc functions
 * return the id of the logical sectors, the id will never change and can be stored for using
 * during the program execution, the actual sector address is handled automatically by the
 * wear leveling algorithm
 * 
 * *** Implementation ***
 * The first () sectors are used to store the logical sectors id to real sector address relation,
 * for more information check the ___ function
 * 
 * @param lower_bound Lowest sector id available to library (inclusive)
 * @param logical_sectors_count How many logical sectors the library will have available
 * @param group_by choose if the library should group sectors by 1, 8, 16 or 64
*/
void init_flash_lib(uint32_t lower_bound, uint16_t logical_sectors_count, uint8_t group_by) {
    _logical_sectors_count = logical_sectors_count;
    _lower_bound = lower_bound;
    _upper_bound = lower_bound + logical_sectors_count * group_by;
    _group_by = group_by;
    
    srand(time_us_32());
    sectors = create_hash_map();
}

void free_all_sectors() {
    reset_hash_map(sectors);
}

void deinit_flash_lib() {
    free_all_sectors();
}

/**
 * @brief Gets random sector id that is not already allocated
 * 
 * It is important to check first if there are available sectors with the
 * _can_allocate_sector_number() function before calling this
 * 
 * First this function generates a random number within the allowed sector range,
 * if the sector is already allocated, it goes upwards in the sector id until it finds
 * a free sector, if none where found, it tries going downwards instead
 * 
 * @return free sector id between _lower_bound and _upper_bound (both inclusive)
*/
uint16_t _get_random_sector_id() {
    uint16_t random_sector = (rand() % (_upper_bound - _lower_bound + 1)) + _lower_bound;

    // Check upwards
    for (uint16_t sector = random_sector; sector <= _upper_bound; ++sector) {
        if (!is_key_in_map(sectors, sector)) {
            return sector;
        }
    }

    // Check downwards
    for (uint16_t sector = random_sector-1; sector >= _lower_bound; --sector) {
        if (!is_key_in_map(sectors, sector)) {
            return sector;
        }
    }

    // No available sector found
}

/**
 * @brief Checks if the given number of sector fits in the range gave to the init function
 * 
 * @param sector_number Amount of sectors to check if they fit in the given range
 * 
 * @return true if there is space for the sectors, false if given amount doesn't fit
*/
bool _can_allocate_sector_number(uint16_t sector_number) {
    uint16_t max_sectors_count = _upper_bound - _lower_bound + 1;
    if ((sectors->count + sector_number) > max_sectors_count) {
        return false;
    }
    return true;
}

/**
 * @brief Allocate a number of sectors
 * 
 * First this function checks if the given number fits in the lower and upper bounds of
 * the allowed sectors.
 * Second it reallocated the sector list to recieve the new sectors.
 * Then a free sector is found and saved
 * 
 * @param sector Array of uint32_t of length sector_number that will be used to save the
 * newly allocated sectors
 * @param sector_number Amount of sectors to be allocated
 * 
 * @return true if successful
*/
bool alloc_multiple_sectors(uint16_t *sector, uint16_t sector_number) {
    if (!_can_allocate_sector_number(sector_number)) {
        return false;
    }

    for (uint16_t sector_index = 0; sector_index < sector_number; ++sector_index) {
        uint16_t sector_address = _get_random_sector_id();
        sector[sector_index] = sectors->count;
        put_in_map(sectors, sector_address, sectors->count);
    }

    return true;
}

/**
 * @brief
 * 
 * 
 * @param sector - pointer to 
*/
bool alloc_sector(uint16_t *sector) {
    return alloc_multiple_sectors(sector, 1);
}

/**
 * @brief Removes multiple sector from the sectors list and reallocates the list.
 *
 * This function iterates over an array of sector IDs and removes each one from
 * the global sectors list by calling the remove_single_sector function. The list
 * is reallocated to a smaller size as needed.
 *
 * @param sector An array of sector IDs to be removed.
 * @param sector_count The number of sector IDs in the array.
 */
void free_multiple_sectors(uint16_t* sector, uint16_t sector_count) {
    remove_multiple_by_value(sectors, sector[0], sector[sector_count-1]);
}

/**
 * @brief Removes a single sector from the sectors list and reallocates the list.
 *
 * @param sector_id The sector ID to be removed.
 */
void free_sector(uint16_t sector) {
    free_multiple_sectors(&sector, 1);
}

/**
 * @brief Gets the pointer to the sector at the offset given
 * 
 * @param sector logical sector ID to read
 * @param logical_sector_offset offset from the beginning of the sector
 * 
 * @return pointer to the memory address
*/
uint8_t * read_sector(uint16_t sector, uint32_t logical_sector_offset) {
    uint32_t fisical_sector_address = get_key_from_value(sectors, sector) * FLASH_SECTOR_SIZE;
    return (uint8_t *) (fisical_sector_address + logical_sector_offset + XIP_BASE);
}

void write_sector(uint16_t sector, uint32_t logical_sector_offset, const uint8_t *data, uint32_t count) {
    uint32_t fisical_sector_address = get_key_from_value(sectors, sector) * FLASH_SECTOR_SIZE;

    uint32_t irq_status = save_and_disable_interrupts();

    flash_range_erase(fisical_sector_address, FLASH_SECTOR_SIZE * _group_by);
    flash_range_program(fisical_sector_address, data, count);

    restore_interrupts(irq_status);
}

#include "pico/time.h"
#include <stdio.h>
void flash_lib_example() {
    gpio_init(8);
    gpio_set_dir(8, true);

    init_flash_lib(120, 15, GROUP_BY_1);
    uint16_t sector[20];
    uint16_t sector_count = 5;

    absolute_time_t start_time;
    absolute_time_t end_time;
    int64_t elapsed_time;
    
    start_time = get_absolute_time();
    
    alloc_multiple_sectors(sector, sector_count);
    
    // End timing
    end_time = get_absolute_time();
    elapsed_time = absolute_time_diff_us(start_time, end_time);
    printf("Elapsed time: %lld microseconds\n", elapsed_time);
    
    // start_time = get_absolute_time();
    
    // free_multiple_sectors(sector, sector_count);
    // // free_all_sectors();
    
    // // End timing
    // end_time = get_absolute_time();
    // elapsed_time = absolute_time_diff_us(start_time, end_time);
    // printf("Elapsed time: %lld microseconds\n", elapsed_time);

    for (uint8_t i = 0; i < sector_count; i++) {
        printf("id: %d\n", sector[i]);
    }

    for (uint8_t i = 0; i < sector_count; i++) {
        printf("Key: %d value: %d\n", get_key_from_value(sectors, sector[i]), sector[i]);
    }

    printf("Elapsed time: %lld microseconds\n", elapsed_time);
    printf("Size in memory: %.2fkb + %.2fkb\n", 1.0*get_hash_map_memory_usage((sectors))/1024, 1.0 * sizeof(sector)/1024);

    sleep_ms(50);

    uint8_t *data = read_sector(sector[0], 0);

    for (u_int8_t i = 0; i < 15; i++) {
        printf("%x, ", data[i]);
    }
    printf("%x\n", data[15]);

    uint8_t test_data[256] = {0xFA, 0xCA, 0xDA, 0xAA, 0xAA, 0x27, 0x06, 0x21};
    write_sector(sector[0], 0, test_data, FLASH_PAGE_SIZE);

    printf("Id: %d\n", get_key_from_value(sectors, sector[0]));
    for (u_int8_t i = 0; i < 15; i++) {
        printf("%x, ", data[i]);
    }
    printf("%x\n", data[15]);
    

    free_all_sectors();
}
