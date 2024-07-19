#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "flash_lib.h"

#define MEMORY_SIGNATURE 0x27062021
#define SIGNATURE_SIZE_BYTES 4

typedef struct SectorHeader {
    uint32_t signature;
    uint16_t logicalID;
    uint16_t writeCount;
} SectorHeader;

uint32_t _lower_bound;
uint32_t _upper_bound;
uint16_t _logical_sectors_count;
uint8_t _group_by;

uint16_t _get_random_sector_addr();
uint8_t * get_sector_read_pointer(uint32_t physical_sector_address);
void init_sectors();
void _write_sector_by_physical_addr(uint32_t physical_sector_address, const uint8_t *data);
void delete_sectors(uint32_t begin, uint32_t end);

/**
 * @brief Initializes the library
 * 
 * *** Library Use ***
 * The flash memory is devided into sectors, each containing 4096 bytes each, while this
 * library uses 'logical sectors', each logical sector will have a number of sectors
 * defined by the group_by attribute.
 * For example, if group_by is 64, each logical sector will be 4kb * 64 = 256kb in size.
 * 
 * Even though each physical sector has 4kb each, due to the overhead, the first 8 bytes are
 * used to store the header, so effectivly, each logical sector will have 4088 bytes.
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
 * *** Library Implementation ***
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
    _upper_bound = lower_bound + (logical_sectors_count - 1) * group_by;
    _group_by = group_by;
    
    srand(time_us_32());

    init_sectors();
}

void print_buffer(uint8_t *buffer, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        printf("%02x ", buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (size % 16 != 0) {
        printf("\n");
    }
}

bool get_sector_addr_by_logical_id(uint16_t logical_id, uint32_t *physical_addr) {
    uint8_t *read_pointer;
    uint16_t header_logical_id;
    uint8_t header_logical_id_size = sizeof(header_logical_id);
    uint32_t signature;
    uint8_t signature_size = SIGNATURE_SIZE_BYTES;
    for (uint32_t physical_sector = _lower_bound; physical_sector <= _upper_bound; physical_sector += _group_by) {
        read_pointer = get_sector_read_pointer(physical_sector);
        memcpy(&signature, read_pointer, SIGNATURE_SIZE_BYTES);
        if (signature != MEMORY_SIGNATURE) {
            continue;
        }
        read_pointer += signature_size;
        memcpy(&header_logical_id, read_pointer, header_logical_id_size);
        if (header_logical_id == logical_id) {
            if (physical_addr != NULL) {
                *physical_addr = physical_sector;
            }
            return true;
        }
    }
    return false;
}

bool check_sector_has_signature(uint32_t physical_sector) {
    uint8_t *read_pointer = get_sector_read_pointer(physical_sector);
    uint32_t signature;
    memcpy(&signature, read_pointer, SIGNATURE_SIZE_BYTES);
    return signature == MEMORY_SIGNATURE;
}

/**
 * @brief Initialize sectors to a known state
*/
void init_sectors() {
    uint16_t unitialized_sectors_count = 0;
    for (uint32_t physical_sector = _lower_bound; physical_sector <= _upper_bound; physical_sector += _group_by) {
        uint8_t *read_pointer = get_sector_read_pointer(physical_sector);
        uint32_t signature;
        memcpy(&signature, read_pointer, SIGNATURE_SIZE_BYTES);

        if (signature != MEMORY_SIGNATURE) {
            unitialized_sectors_count++;
            continue;
        }

        uint16_t logical_id;
        memcpy(&logical_id, read_pointer + SIGNATURE_SIZE_BYTES, 2);
        if (logical_id >= _logical_sectors_count) {
            delete_sectors(physical_sector, physical_sector);
            unitialized_sectors_count++;
        }
    }

    if (unitialized_sectors_count == 0) {
        return;
    }

    // Checks every logical ID to know which ones needs initialization
    bool found_sector;
    for (uint32_t logical_id = 0; logical_id < _logical_sectors_count; logical_id++) {
        found_sector = get_sector_addr_by_logical_id(logical_id, NULL);

        if (found_sector) {
            continue;
        }

        // Allocates new physical sector for the logical id
        SectorHeader sectorHeader = {
            .signature = MEMORY_SIGNATURE,
            .logicalID = logical_id,
            .writeCount = 1,
        };
        uint8_t headerBuffer[FLASH_PAGE_SIZE];
        memset(headerBuffer, 0xFF, FLASH_PAGE_SIZE);
        memcpy(headerBuffer, &sectorHeader, sizeof(SectorHeader));
        _write_sector_by_physical_addr(_get_random_sector_addr(), headerBuffer);

        // Finished initializing all sectors
        unitialized_sectors_count--;
        if (unitialized_sectors_count == 0) {
            break;
        }
    }
}

/**
 * @brief Gets random sector address that is not already allocated
 * 
 * It is important to check first if there are available sectors with the
 * _can_allocate_sector_number() function before calling this
 * 
 * First this function generates a random number within the allowed sector range,
 * if the sector is already allocated, it goes upwards in the sector address until it finds
 * a free sector, if none where found, it tries going downwards instead
 * 
 * @return free sector address between _lower_bound and _upper_bound (both inclusive)
*/
uint16_t _get_random_sector_addr() {
    uint16_t random_sector = (rand() % (_upper_bound - _lower_bound + 1)) + _lower_bound;

    // Check upwards
    uint8_t *read_pointer;
    uint16_t header_logical_id;
    uint8_t header_logical_id_size = sizeof(header_logical_id);
    for (uint16_t physical_sector = random_sector; physical_sector <= _upper_bound; ++physical_sector) {
        if (!check_sector_has_signature(physical_sector)) {
            return physical_sector;
        }
    }

    // Check downwards
    for (uint16_t physical_sector = random_sector-1; physical_sector >= _lower_bound; --physical_sector) {
        if (!check_sector_has_signature(physical_sector)) {
            return physical_sector;
        }
    }

    // No available sector found
}

uint32_t physical_sector_addr_to_memory_addr(uint32_t sector_addr) {
    return sector_addr * FLASH_SECTOR_SIZE;
}

uint8_t * get_sector_read_pointer(uint32_t physical_sector_id) {
    return (uint8_t *) (physical_sector_addr_to_memory_addr(physical_sector_id) + XIP_BASE);
}

/**
 * @brief Gets the pointer to the sector at the given offset
 * 
 * @param sector logical sector ID to read
 * @param logical_sector_offset offset from the beginning of the sector
 * 
 * @return pointer to the memory address
*/
uint8_t * read_sector(uint16_t sector, uint32_t logical_sector_offset) {
    uint32_t physical_sector_address;
    get_sector_addr_by_logical_id(sector, &physical_sector_address);
    return get_sector_read_pointer(physical_sector_address) + logical_sector_offset;
}

/**
 * @brief Gets sector header, updates and returns it
*/
void read_and_update_header(uint32_t physical_sector_id, SectorHeader *sectorHeader) {
    uint8_t *read_pointer = get_sector_read_pointer(physical_sector_id);
    memcpy(sectorHeader, read_pointer, sizeof(SectorHeader));
    sectorHeader->writeCount++;
}

void _write_sector_by_physical_addr(uint32_t physical_sector_address, const uint8_t *data) {
    physical_sector_address = physical_sector_addr_to_memory_addr(physical_sector_address);

    uint32_t irq_status = save_and_disable_interrupts();

    flash_range_erase(physical_sector_address, FLASH_SECTOR_SIZE);
    flash_range_program(physical_sector_address, data, FLASH_PAGE_SIZE);

    restore_interrupts(irq_status);
}

void write_sector(uint16_t sector, uint32_t logical_sector_offset, const uint8_t *data, uint32_t count) {
    uint32_t physical_sector_address;
    get_sector_addr_by_logical_id(sector, &physical_sector_address);

    SectorHeader sectorHeader;
    uint8_t headerBuffer[FLASH_PAGE_SIZE];
    uint8_t headerSize = sizeof(SectorHeader);
    read_and_update_header(physical_sector_address, &sectorHeader);
    memset(headerBuffer, 0xFF, FLASH_PAGE_SIZE);
    memcpy(headerBuffer, &sectorHeader, headerSize);

    physical_sector_address = physical_sector_addr_to_memory_addr(physical_sector_address);

    // uint32_t writeAddress = physical_sector_address + headerSize + logical_sector_offset;
    memcpy(headerBuffer + headerSize, data, count-headerSize);

    uint32_t irq_status = save_and_disable_interrupts();

    flash_range_erase(physical_sector_address, FLASH_SECTOR_SIZE);
    flash_range_program(physical_sector_address, headerBuffer, headerSize);
    // flash_range_program(writeAddress, data, count);

    restore_interrupts(irq_status);
}

void unsafe_write_sector(uint32_t physical_sector, uint8_t *data) {
    flash_range_program(physical_sector_addr_to_memory_addr(physical_sector), data, FLASH_PAGE_SIZE);
}

void delete_sectors(uint32_t begin, uint32_t end) {
    uint8_t headerBuffer[FLASH_PAGE_SIZE];
    uint8_t cleanHeader[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memset(headerBuffer, 0xFF, FLASH_PAGE_SIZE);
    memcpy(headerBuffer, &cleanHeader, sizeof(cleanHeader));

    uint8_t *read = get_sector_read_pointer(_lower_bound);

    uint32_t irq_status = save_and_disable_interrupts();
    
    for (uint32_t physical_sector = begin; physical_sector <= end; ++physical_sector) {
        unsafe_write_sector(physical_sector, headerBuffer);
    }

    restore_interrupts(irq_status);
}

/**
 * @brief Erases sector header
*/
void delete_all_sectors() {
    delete_sectors(_lower_bound, _upper_bound);
}

#include "pico/time.h"
void flash_lib_example() {
    absolute_time_t start_time;
    absolute_time_t end_time;
    float elapsed_time;

    uint16_t sector_to_write = 0;
    uint32_t my_physical_sector;
    uint8_t data_to_write[FLASH_PAGE_SIZE];
    memset(data_to_write, 0xFF, FLASH_PAGE_SIZE);
    data_to_write[0] = 0x0A;
    data_to_write[1] = 0xFA;
    data_to_write[2] = 0xCA;
    data_to_write[3] = 0xDA;

    start_time = get_absolute_time();
    // *** Code ***
    init_flash_lib(100, 500, GROUP_BY_1);
    // *** Code ***
    end_time = get_absolute_time();
    elapsed_time = 1.0*absolute_time_diff_us(start_time, end_time)/1000;
    printf("Time to init library: %.3fms\n", elapsed_time);


    start_time = get_absolute_time();
    // *** Code ***
    get_sector_addr_by_logical_id(sector_to_write, &my_physical_sector);
    // *** Code ***
    end_time = get_absolute_time();
    elapsed_time = 1.0*absolute_time_diff_us(start_time, end_time);
    printf("Time to find sector: %.2fus\n", elapsed_time);
    printf("Logical id: %d At physical addr at: %d\n", sector_to_write, my_physical_sector);


    // start_time = get_absolute_time();
    // // *** Code ***
    // write_sector(sector_to_write, 0, data_to_write, FLASH_PAGE_SIZE);
    // // *** Code ***
    // end_time = get_absolute_time();
    // elapsed_time = 1.0*absolute_time_diff_us(start_time, end_time)/1000;
    // printf("Time to write to sector: %.2fms\n", elapsed_time);


    start_time = get_absolute_time();
    // *** Code ***
    get_sector_addr_by_logical_id(1 << 15, &my_physical_sector);
    // *** Code ***
    end_time = get_absolute_time();
    elapsed_time = 1.0*absolute_time_diff_us(start_time, end_time);
    printf("Time to read all headers: %.2fus\n", elapsed_time);
}
