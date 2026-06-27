#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "memory_manager.h"
#include "trace_reader.h"

/*
 * Representa o simulador completo.
 * Coordena a leitura do trace e o
 * gerenciamento da memória virtual.
 */
typedef struct {

    TraceReader reader;
    MemoryManager manager;

} Simulator;

/* Inicializa o simulador */
int simulator_init(Simulator *simulator,
                   const char *trace_file,
                   ReplacementPolicyType policy,
                   const char *swap_file);

/* Executa toda a simulação */
int simulator_run(Simulator *simulator);

/* Libera os recursos utilizados */
void simulator_destroy(Simulator *simulator);

#endif