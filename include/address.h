#ifndef ADDRESS_H
#define ADDRESS_H

#include <stdint.h>

#define PAGE_OFFSET_BITS 12
#define PAGE_SIZE (1ULL << PAGE_OFFSET_BITS)

#define PAGE_TABLE_LEVELS 4
#define PAGE_TABLE_INDEX_BITS 9
#define PAGE_TABLE_ENTRIES (1ULL << PAGE_TABLE_INDEX_BITS)
#define PAGE_TABLE_INDEX_MASK (PAGE_TABLE_ENTRIES - 1)

uint64_t address_page_number(uint64_t address);
uint16_t address_offset(uint64_t address);
uint16_t address_level_index(uint64_t address, int level);

#endif