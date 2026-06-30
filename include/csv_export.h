#ifndef CSV_EXPORT_H
#define CSV_EXPORT_H

#include <stdbool.h>
#include <stdint.h>

#include "cli.h"
#include "virtual_memory.h"

/*
 * Exporta o resultado de uma simulação para CSV.
 *
 * Retorna:
 *  0 em caso de sucesso;
 * -1 em caso de argumento inválido ou erro de escrita.
 */
int csv_export_result(
    const char *path,
    const CliOptions *options,
    const VirtualMemory *memory,
    uint64_t processed_accesses,
    bool append
);

#endif