#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "page_table.h"
#include "tlb.h"
#include "frame.h"
#include "replacement.h"
#include "swap.h"
#include "statistics.h"

/*
 * Gerenciador central da memória virtual.
 * Integra todos os módulos do simulador.
 */
typedef struct {

    PageTable page_table;
    TLB tlb;

    Frame frames[FRAME_COUNT];

    ReplacementPolicy replacement;

    Swap swap;

    Statistics statistics;

} MemoryManager;

/* Inicializa todos os componentes */
int memory_manager_init(MemoryManager *manager,
                        ReplacementPolicyType policy,
                        const char *swap_file);

/* Libera os recursos utilizados */
void memory_manager_destroy(MemoryManager *manager);

/* Realiza um acesso à memória virtual */
TranslationStatus memory_manager_access(
    MemoryManager *manager,
    unsigned long virtual_address,
    AccessType access_type);

#endif