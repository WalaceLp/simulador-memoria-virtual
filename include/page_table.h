#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <stddef.h>

typedef struct PageTable PageTable;

PageTable *page_table_create(void);
void page_table_destroy(PageTable *table);

#endif