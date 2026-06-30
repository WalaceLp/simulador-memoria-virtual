#ifndef CLI_H
#define CLI_H

#include <stdbool.h>
#include <stddef.h>

#include "replacement.h"

typedef struct {
    const char *trace_path;
    const char *swap_path;

    size_t frame_count;
    size_t tlb_entries;
    size_t swap_slots;

    int pid;

    ReplacementPolicyType policy;

    bool remove_swap_on_destroy;
    bool show_help;
} CliOptions;

void cli_options_init(CliOptions *options);

/*
 * Retornos:
 *  0: argumentos válidos;
 *  1: ajuda solicitada;
 * -1: erro nos argumentos.
 */
int cli_parse(
    int argc,
    char **argv,
    CliOptions *options
);

void cli_print_usage(const char *program_name);

#endif