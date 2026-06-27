#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "trie.h"

/*
 * Representa a tabela de páginas do sistema.
 * Internamente utiliza uma Trie para armazenar
 * o mapeamento entre páginas virtuais e frames.
 */
typedef struct {

    Trie *trie;

} PageTable;

/* Cria uma nova tabela de páginas */
PageTable *page_table_create(void);

/* Libera a tabela de páginas */
void page_table_destroy(PageTable *page_table);

/* Mapeia uma página virtual para um frame */
bool page_table_map(PageTable *page_table,
                    int virtual_page,
                    int frame);

/* Remove o mapeamento de uma página */
bool page_table_unmap(PageTable *page_table,
                      int virtual_page);

/* Traduz uma página virtual para um frame */
bool page_table_translate(PageTable *page_table,
                          int virtual_page,
                          int *frame);

#endif