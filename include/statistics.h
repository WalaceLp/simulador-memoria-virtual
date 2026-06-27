#ifndef STATISTICS_H
#define STATISTICS_H

/*
 * Armazena todas as estatísticas da simulação.
 */
typedef struct {

    unsigned long total_accesses;

    unsigned long tlb_hits;
    unsigned long tlb_misses;

    unsigned long page_faults;

    unsigned long swap_reads;
    unsigned long swap_writes;

    unsigned long replacements;

} Statistics;

/* Inicializa todas as estatísticas */
void statistics_init(Statistics *stats);

/* Calcula o percentual de acertos da TLB */
double statistics_tlb_hit_ratio(const Statistics *stats);

/* Exibe as estatísticas finais */
void statistics_print(const Statistics *stats);

#endif