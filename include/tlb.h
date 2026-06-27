#ifndef TLB_H
#define TLB_H

#include "common.h"

/*
 * Representa uma entrada da TLB.
 */
typedef struct {

    int virtual_page;

    int frame_number;

    bool valid;

} TLBEntry;

/*
 * Representa toda a TLB.
 */
typedef struct {

    TLBEntry entries[TLB_SIZE];

} TLB;

/* Cria uma nova TLB */
void tlb_init(TLB *tlb);

/* Procura uma página virtual */
bool tlb_lookup(TLB *tlb,
                int virtual_page,
                int *frame);

/* Insere uma tradução */
void tlb_insert(TLB *tlb,
                int virtual_page,
                int frame);

/* Remove uma entrada */
void tlb_remove(TLB *tlb,
                int virtual_page);

#endif