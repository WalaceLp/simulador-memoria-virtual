#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"

#define DEFAULT_FRAME_COUNT 4
#define DEFAULT_TLB_ENTRIES 16
#define DEFAULT_SWAP_SLOTS 64
#define DEFAULT_PID 1
#define DEFAULT_SWAP_PATH "bin/vmsim.swap"

void cli_options_init(CliOptions *options)
{
    if (options == NULL) {
        return;
    }

    options->trace_path = NULL;
    options->swap_path = DEFAULT_SWAP_PATH;

    options->frame_count = DEFAULT_FRAME_COUNT;
    options->tlb_entries = DEFAULT_TLB_ENTRIES;
    options->swap_slots = DEFAULT_SWAP_SLOTS;

    options->pid = DEFAULT_PID;

    options->policy = REPLACEMENT_FIFO;

    options->remove_swap_on_destroy = true;
    options->show_help = false;
}

static int cli_parse_size(
    const char *text,
    size_t *value
)
{
    if (
        text == NULL ||
        value == NULL ||
        *text == '\0' ||
        *text == '-'
    ) {
        return -1;
    }

    errno = 0;

    char *end = NULL;

    unsigned long long parsed = strtoull(
        text,
        &end,
        10
    );

    if (
        errno != 0 ||
        end == text ||
        *end != '\0' ||
        parsed == 0 ||
        parsed > SIZE_MAX
    ) {
        return -1;
    }

    *value = (size_t)parsed;

    return 0;
}

static int cli_parse_pid(
    const char *text,
    int *pid
)
{
    if (
        text == NULL ||
        pid == NULL ||
        *text == '\0'
    ) {
        return -1;
    }

    errno = 0;

    char *end = NULL;

    long parsed = strtol(
        text,
        &end,
        10
    );

    if (
        errno != 0 ||
        end == text ||
        *end != '\0' ||
        parsed < 0 ||
        parsed > INT_MAX
    ) {
        return -1;
    }

    *pid = (int)parsed;

    return 0;
}

static int cli_parse_policy(
    const char *text,
    ReplacementPolicyType *policy
)
{
    if (text == NULL || policy == NULL) {
        return -1;
    }

    if (strcmp(text, "fifo") == 0) {
        *policy = REPLACEMENT_FIFO;
        return 0;
    }

    if (strcmp(text, "lru") == 0) {
        *policy = REPLACEMENT_LRU;
        return 0;
    }

    if (strcmp(text, "clock") == 0) {
        *policy = REPLACEMENT_CLOCK;
        return 0;
    }

    if (strcmp(text, "aging") == 0) {
        *policy = REPLACEMENT_AGING;
        return 0;
    }

    return -1;
}

static int cli_requires_value(
    int index,
    int argc
)
{
    return index + 1 < argc;
}

int cli_parse(
    int argc,
    char **argv,
    CliOptions *options
)
{
    if (
        argc < 1 ||
        argv == NULL ||
        options == NULL
    ) {
        return -1;
    }

    cli_options_init(options);

    for (int index = 1; index < argc; index++) {
        const char *argument = argv[index];

        if (
            strcmp(argument, "--help") == 0 ||
            strcmp(argument, "-h") == 0
        ) {
            options->show_help = true;
            return 1;
        }

        if (strcmp(argument, "--keep-swap") == 0) {
            options->remove_swap_on_destroy = false;
            continue;
        }

        if (strcmp(argument, "--trace") == 0) {
            if (!cli_requires_value(index, argc)) {
                return -1;
            }

            options->trace_path = argv[++index];
            continue;
        }

        if (strcmp(argument, "--swap") == 0) {
            if (!cli_requires_value(index, argc)) {
                return -1;
            }

            options->swap_path = argv[++index];
            continue;
        }

        if (strcmp(argument, "--frames") == 0) {
            if (
                !cli_requires_value(index, argc) ||
                cli_parse_size(
                    argv[++index],
                    &options->frame_count
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        if (strcmp(argument, "--tlb") == 0) {
            if (
                !cli_requires_value(index, argc) ||
                cli_parse_size(
                    argv[++index],
                    &options->tlb_entries
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        if (strcmp(argument, "--swap-slots") == 0) {
            if (
                !cli_requires_value(index, argc) ||
                cli_parse_size(
                    argv[++index],
                    &options->swap_slots
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        if (strcmp(argument, "--pid") == 0) {
            if (
                !cli_requires_value(index, argc) ||
                cli_parse_pid(
                    argv[++index],
                    &options->pid
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        if (strcmp(argument, "--policy") == 0) {
            if (
                !cli_requires_value(index, argc) ||
                cli_parse_policy(
                    argv[++index],
                    &options->policy
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        return -1;
    }

    if (options->trace_path == NULL) {
        return -1;
    }

    return 0;
}

void cli_print_usage(const char *program_name)
{
    const char *name =
        program_name != NULL
            ? program_name
            : "vmsim";

    printf(
        "Uso:\n"
        "  %s --trace <arquivo> [opcoes]\n\n"
        "Opcoes:\n"
        "  --trace <arquivo>       Trace a ser executado\n"
        "  --frames <quantidade>   Quadros fisicos, padrao: %d\n"
        "  --policy <politica>     fifo, lru, clock ou aging\n"
        "  --tlb <quantidade>      Entradas da TLB, padrao: %d\n"
        "  --swap <arquivo>        Arquivo de swap\n"
        "  --swap-slots <numero>   Slots do swap, padrao: %d\n"
        "  --pid <numero>          PID simulado, padrao: %d\n"
        "  --keep-swap             Nao remove o swap ao finalizar\n"
        "  --help, -h              Exibe esta ajuda\n",
        name,
        DEFAULT_FRAME_COUNT,
        DEFAULT_TLB_ENTRIES,
        DEFAULT_SWAP_SLOTS,
        DEFAULT_PID
    );
}