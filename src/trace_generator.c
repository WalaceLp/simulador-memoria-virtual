#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "address.h"
#include "trace_generator.h"

#define DEFAULT_PAGE_COUNT 16
#define DEFAULT_ACCESS_COUNT 1000
#define DEFAULT_SEED 1ULL
#define DEFAULT_BASE_ADDRESS 0x1000ULL
#define DEFAULT_WRITE_PERCENTAGE 20U

typedef struct {
    uint64_t state;
} TraceRandom;

void trace_generator_options_init(
    TraceGeneratorOptions *options
)
{
    if (options == NULL) {
        return;
    }

    options->output_path = NULL;

    options->pattern = TRACE_PATTERN_SEQUENTIAL;

    options->page_count = DEFAULT_PAGE_COUNT;
    options->access_count = DEFAULT_ACCESS_COUNT;

    options->seed = DEFAULT_SEED;
    options->base_address = DEFAULT_BASE_ADDRESS;

    options->write_percentage =
        DEFAULT_WRITE_PERCENTAGE;

    options->show_help = false;
}

static uint64_t trace_random_next(
    TraceRandom *random
)
{
    random->state =
        random->state * 6364136223846793005ULL +
        1442695040888963407ULL;

    return random->state;
}

static int trace_parse_size(
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

static int trace_parse_uint64(
    const char *text,
    uint64_t *value
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
        0
    );

    if (
        errno != 0 ||
        end == text ||
        *end != '\0'
    ) {
        return -1;
    }

    *value = (uint64_t)parsed;

    return 0;
}

static int trace_parse_percentage(
    const char *text,
    unsigned int *value
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

    unsigned long parsed = strtoul(
        text,
        &end,
        10
    );

    if (
        errno != 0 ||
        end == text ||
        *end != '\0' ||
        parsed > 100UL ||
        parsed > UINT_MAX
    ) {
        return -1;
    }

    *value = (unsigned int)parsed;

    return 0;
}

static int trace_parse_pattern(
    const char *text,
    TracePattern *pattern
)
{
    if (text == NULL || pattern == NULL) {
        return -1;
    }

    if (strcmp(text, "sequential") == 0) {
        *pattern = TRACE_PATTERN_SEQUENTIAL;
        return 0;
    }

    if (strcmp(text, "random") == 0) {
        *pattern = TRACE_PATTERN_RANDOM;
        return 0;
    }

    return -1;
}

static bool trace_has_next_argument(
    int index,
    int argc
)
{
    return index + 1 < argc;
}

int trace_generator_parse(
    int argc,
    char **argv,
    TraceGeneratorOptions *options
)
{
    if (
        argc < 1 ||
        argv == NULL ||
        options == NULL
    ) {
        return -1;
    }

    trace_generator_options_init(options);

    for (int index = 1; index < argc; index++) {
        const char *argument = argv[index];

        if (
            strcmp(argument, "--help") == 0 ||
            strcmp(argument, "-h") == 0
        ) {
            options->show_help = true;
            return 1;
        }

        if (strcmp(argument, "--output") == 0) {
            if (!trace_has_next_argument(index, argc)) {
                return -1;
            }

            options->output_path = argv[++index];
            continue;
        }

        if (strcmp(argument, "--pattern") == 0) {
            if (
                !trace_has_next_argument(index, argc) ||
                trace_parse_pattern(
                    argv[++index],
                    &options->pattern
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        if (strcmp(argument, "--pages") == 0) {
            if (
                !trace_has_next_argument(index, argc) ||
                trace_parse_size(
                    argv[++index],
                    &options->page_count
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        if (strcmp(argument, "--accesses") == 0) {
            if (
                !trace_has_next_argument(index, argc) ||
                trace_parse_size(
                    argv[++index],
                    &options->access_count
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        if (strcmp(argument, "--seed") == 0) {
            if (
                !trace_has_next_argument(index, argc) ||
                trace_parse_uint64(
                    argv[++index],
                    &options->seed
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        if (strcmp(argument, "--base") == 0) {
            if (
                !trace_has_next_argument(index, argc) ||
                trace_parse_uint64(
                    argv[++index],
                    &options->base_address
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        if (strcmp(argument, "--write-rate") == 0) {
            if (
                !trace_has_next_argument(index, argc) ||
                trace_parse_percentage(
                    argv[++index],
                    &options->write_percentage
                ) != 0
            ) {
                return -1;
            }

            continue;
        }

        return -1;
    }

    if (options->output_path == NULL) {
        return -1;
    }

    return 0;
}

const char *trace_generator_pattern_name(
    TracePattern pattern
)
{
    switch (pattern) {
        case TRACE_PATTERN_SEQUENTIAL:
            return "sequential";

        case TRACE_PATTERN_RANDOM:
            return "random";

        default:
            return "invalid";
    }
}

static bool trace_address_range_is_valid(
    const TraceGeneratorOptions *options
)
{
    if (
        options == NULL ||
        options->page_count == 0
    ) {
        return false;
    }

    uint64_t last_page_index =
        (uint64_t)(options->page_count - 1);

    if (
        last_page_index >
        (UINT64_MAX - options->base_address) /
        PAGE_SIZE
    ) {
        return false;
    }

    return true;
}

static uint64_t trace_choose_page(
    const TraceGeneratorOptions *options,
    TraceRandom *random,
    size_t access_index
)
{
    if (options->pattern == TRACE_PATTERN_SEQUENTIAL) {
        return (uint64_t)(
            access_index % options->page_count
        );
    }

    return trace_random_next(random) %
           options->page_count;
}

static char trace_choose_operation(
    const TraceGeneratorOptions *options,
    TraceRandom *random
)
{
    uint64_t value =
        trace_random_next(random) % 100ULL;

    if (value < options->write_percentage) {
        return 'W';
    }

    return 'R';
}

int trace_generator_generate(
    const TraceGeneratorOptions *options
)
{
    if (
        options == NULL ||
        options->output_path == NULL ||
        options->page_count == 0 ||
        options->access_count == 0 ||
        options->write_percentage > 100 ||
        (
            options->pattern !=
                TRACE_PATTERN_SEQUENTIAL &&
            options->pattern !=
                TRACE_PATTERN_RANDOM
        ) ||
        !trace_address_range_is_valid(options)
    ) {
        return -1;
    }

    FILE *file = fopen(options->output_path, "w");

    if (file == NULL) {
        return -1;
    }

    TraceRandom random = {
        .state = options->seed
    };

    for (
        size_t access_index = 0;
        access_index < options->access_count;
        access_index++
    ) {
        uint64_t page_index = trace_choose_page(
            options,
            &random,
            access_index
        );

        uint64_t address =
            options->base_address +
            page_index * PAGE_SIZE;

        char operation = trace_choose_operation(
            options,
            &random
        );

        if (
            fprintf(
                file,
                "%c 0x%016" PRIx64 "\n",
                operation,
                address
            ) < 0
        ) {
            fclose(file);
            return -1;
        }
    }

    if (fclose(file) != 0) {
        return -1;
    }

    return 0;
}

void trace_generator_print_usage(
    const char *program_name
)
{
    const char *name =
        program_name != NULL
            ? program_name
            : "tracegen";

    printf(
        "Uso:\n"
        "  %s --output <arquivo> [opcoes]\n\n"
        "Opcoes:\n"
        "  --output <arquivo>       Arquivo de trace gerado\n"
        "  --pattern <padrao>       sequential ou random\n"
        "  --pages <quantidade>     Paginas distintas, padrao: %d\n"
        "  --accesses <quantidade>  Total de acessos, padrao: %d\n"
        "  --write-rate <0-100>     Percentual de escritas, padrao: %u\n"
        "  --seed <numero>          Semente aleatoria, padrao: %" PRIu64 "\n"
        "  --base <endereco>        Endereco base, padrao: 0x%" PRIx64 "\n"
        "  --help, -h               Exibe esta ajuda\n",
        name,
        DEFAULT_PAGE_COUNT,
        DEFAULT_ACCESS_COUNT,
        DEFAULT_WRITE_PERCENTAGE,
        (uint64_t)DEFAULT_SEED,
        (uint64_t)DEFAULT_BASE_ADDRESS
    );
}