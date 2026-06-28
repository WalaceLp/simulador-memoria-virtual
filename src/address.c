#include "address.h"

uint64_t address_page_number(uint64_t address)
{
    return address >> PAGE_OFFSET_BITS;
}

uint16_t address_offset(uint64_t address)
{
    return (uint16_t)(address & (PAGE_SIZE - 1));
}

uint16_t address_level_index(uint64_t address, int level)
{
    if (level < 0 || level >= PAGE_TABLE_LEVELS) {
        return 0;
    }

    int shift = PAGE_OFFSET_BITS +
                PAGE_TABLE_INDEX_BITS * (PAGE_TABLE_LEVELS - 1 - level);

    return (uint16_t)((address >> shift) & PAGE_TABLE_INDEX_MASK);
}