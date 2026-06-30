#ifndef TRACE_H
#define TRACE_H

#include <stddef.h>
#include <stdint.h>

#include "process.h"
#include "virtual_memory.h"

typedef enum {
    TRACE_ACCESS_READ = 0,
    TRACE_ACCESS_WRITE = 1
} TraceAccessType;

typedef struct {
    TraceAccessType type;
    uint64_t address;
    size_t size;
} TraceEntry;

typedef struct TraceReader TraceReader;

TraceReader *trace_reader_open(const char *path);

void trace_reader_close(TraceReader *reader);

/*
 * Retornos:
 *  1: uma entrada foi lida;
 *  0: fim do arquivo;
 * -1: linha inválida ou erro de leitura.
 */
int trace_reader_next(
    TraceReader *reader,
    TraceEntry *entry
);

int trace_replay(
    VirtualMemory *memory,
    Process *process,
    const char *path,
    uint64_t *processed_accesses
);

#endif