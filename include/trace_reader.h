#ifndef TRACE_READER_H
#define TRACE_READER_H

#include <stdbool.h>
#include <stdio.h>

/*
 * Tipo de operação presente no trace.
 */
typedef enum {

    TRACE_READ,
    TRACE_WRITE

} TraceOperation;

/*
 * Representa uma única linha do arquivo
 * de trace.
 */
typedef struct {

    TraceOperation operation;
    unsigned long address;

} TraceEntry;

/*
 * Representa um arquivo de trace aberto.
 */
typedef struct {

    FILE *file;

} TraceReader;

/* Abre um arquivo de trace */
int trace_reader_open(TraceReader *reader,
                      const char *filename);

/* Fecha o arquivo de trace */
void trace_reader_close(TraceReader *reader);

/* Lê a próxima operação do trace.
 * Retorna:
 *   true  -> operação lida com sucesso
 *   false -> fim do arquivo
 */
bool trace_reader_next(TraceReader *reader,
                       TraceEntry *entry);

#endif