#include "flash_lib.h"
// #include "hardware/flash.h"
// #include "hardware/sync.h"
#include <stdlib.h>

#define SECTOR_POSITION_MASK 0x3FFFF

/**
 * Array of uint32_t that contains that maps the sector ID, to the sector number and era counter
 * 
 * Each entry consists of 32 bits, of which:
 * - 0 to 17 -> Sector number
 * - 18 to 31 -> Erase counter of sector
 * The array index is the sector ID
 * 
 * This array automatically resizes when allocating more sectors, the maximum amount of sectors
 * is 2^18-1 = 262,143 sector, or a little over 8 gigabits
*/
uint32_t *sectors;
uint32_t total_sectors = 0;

uint32_t _lower_bound;
uint32_t _upper_bound;

/**
 * @brief Initializes the library
 * 
 * @param lower_bound Lowest sector id available to library (inclusive)
 * @param upper_bound highest sector id available to library (inclusive)
*/
void init_flash_lib(uint32_t lower_bound, uint32_t upper_bound) {
    _lower_bound = lower_bound;
    _upper_bound = upper_bound;
    
    // init_random();
    srand(time_us_32());
}

void deinit_flash_lib() {
    free(sectors);
}

/**
 * @brief Checks if sector is already allocated
 * 
 * @param sector Sector id to check
 * 
 * @return true if is already allocated, false if free
*/
bool _is_sector_allocated(uint32_t sector) {
    for (uint32_t i = 0; i < total_sectors; i++) {
        if (sectors[i] == sector) {
            return true;
        }
    }
    return false;
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
uint32_t _get_random_sector_id() {
    uint32_t random_sector = (rand() % (_upper_bound - _lower_bound + 1)) + _lower_bound;

    // Check upwards
    for (uint32_t sector = random_sector; sector <= _upper_bound; ++sector) {
        if (!_is_sector_allocated(sector)) {
            return sector;
        }
    }

    // Check downwards
    for (uint32_t sector = random_sector-1; sector >= _lower_bound; --sector) {
        if (!_is_sector_allocated(sector)) {
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
bool _can_allocate_sector_number(uint32_t sector_number) {
    uint32_t max_sectors_count = _upper_bound - _lower_bound + 1;
    if ((total_sectors + sector_number) >= max_sectors_count) {
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
bool alloc_multiple_sectors(uint32_t *sector, uint32_t sector_number) {
    if (!_can_allocate_sector_number(sector_number)) {
        return false;
    }

    uint32_t *new_list = (uint32_t *)realloc(sectors, (total_sectors + sector_number) * sizeof(uint32_t));

    if (new_list == NULL) {
        return false;
    }

    sectors = new_list;
    for (uint32_t sector_index = 0; sector_index < sector_number; ++sector_index) {
        uint32_t sector_address = _get_random_sector_id();
        // Saves the first 18 bits of the sector address
        uint32_t absolute_address = total_sectors + sector_index;
        sectors[absolute_address] = sector_address;
        sector[sector_index] = absolute_address;
    }
    total_sectors += sector_number;

    return true;
}

/**
 * @brief
 * 
 * 
 * @param sector - pointer to 
*/
bool alloc_sector(uint32_t *sector) {
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
void free_multiple_sectors(uint32_t* sector, uint32_t sector_count) {
    uint32_t new_count = total_sectors;
    bool *to_remove = (bool *)calloc(sector_count, sizeof(bool));

    for (uint32_t i = 0; i < total_sectors; ++i) {
        if (i != sector[i]) {
            continue;
        }

        for (uint32_t j = 0; j < sector_count; ++j) {
            to_remove[i+j] = true;
            --new_count;
        }

        break;
    }

    // Create a new array with the remaining sectors
    uint32_t *new_sectors = (uint32_t*)malloc(new_count * sizeof(uint32_t));
    uint32_t new_index = 0;
    for (uint32_t i = 0; i < total_sectors; ++i) {
        if (!to_remove[i]) {
            new_sectors[new_index++] = sectors[i];
        }
    }

    // Update global variables
    free(sectors);
    sectors = new_sectors;
    total_sectors = new_count;
    free(to_remove);
}

/**
 * @brief Removes a single sector from the sectors list and reallocates the list.
 *
 * @param sector_id The sector ID to be removed.
 */
void free_sector(uint32_t sector) {
    free_multiple_sectors(&sector, 1);
}

void read_sector() {
    
}

void write_sector() {

}

#include "pico/time.h"
#include <stdio.h>
void flash_lib_example() {
    gpio_init(8);
    gpio_set_dir(8, true);

    init_flash_lib(100, 1100);
    uint32_t sector[1000];
    uint16_t sector_count = 1000;

    absolute_time_t start_time;
    absolute_time_t end_time;
    int64_t elapsed_time;
    
    start_time = get_absolute_time();
    
    alloc_multiple_sectors(sector, sector_count);
    
    // End timing
    end_time = get_absolute_time();
    elapsed_time = absolute_time_diff_us(start_time, end_time);
    printf("Elapsed time: %lld microseconds\n", elapsed_time);
    
    start_time = get_absolute_time();
    
    free_multiple_sectors(sector, sector_count);
    
    // End timing
    end_time = get_absolute_time();
    elapsed_time = absolute_time_diff_us(start_time, end_time);
    printf("Elapsed time: %lld microseconds\n", elapsed_time);
}
