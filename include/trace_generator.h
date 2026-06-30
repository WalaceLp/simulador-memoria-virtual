#ifndef TRACE_GENERATOR_H
#define TRACE_GENERATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    TRACE_PATTERN_SEQUENTIAL = 0,
    TRACE_PATTERN_RANDOM = 1
} TracePattern;

typedef struct {
    const char *output_path;

    TracePattern pattern;

    size_t page_count;
    size_t access_count;

    uint64_t seed;
    uint64_t base_address;

    unsigned int write_percentage;

    bool show_help;
} TraceGeneratorOptions;

void trace_generator_options_init(
    TraceGeneratorOptions *options
);

/*
 * Retornos:
 *  0: argumentos válidos;
 *  1: ajuda solicitada;
 * -1: argumentos inválidos.
 */
int trace_generator_parse(
    int argc,
    char **argv,
    TraceGeneratorOptions *options
);

int trace_generator_generate(
    const TraceGeneratorOptions *options
);

const char *trace_generator_pattern_name(
    TracePattern pattern
);

void trace_generator_print_usage(
    const char *program_name
);

#endif