#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "trace_generator.h"

static void print_generation_summary(
    const TraceGeneratorOptions *options
)
{
    printf("Trace gerado com sucesso.\n");
    printf("Arquivo: %s\n", options->output_path);

    printf(
        "Padrao: %s\n",
        trace_generator_pattern_name(
            options->pattern
        )
    );

    printf(
        "Paginas distintas: %zu\n",
        options->page_count
    );

    printf(
        "Acessos: %zu\n",
        options->access_count
    );

    printf(
        "Percentual de escrita: %u%%\n",
        options->write_percentage
    );

    printf(
        "Semente: %" PRIu64 "\n",
        options->seed
    );

    printf(
        "Endereco base: 0x%" PRIx64 "\n",
        options->base_address
    );
}

int main(int argc, char **argv)
{
    TraceGeneratorOptions options;

    int parse_result = trace_generator_parse(
        argc,
        argv,
        &options
    );

    if (parse_result == 1) {
        trace_generator_print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (parse_result != 0) {
        fprintf(
            stderr,
            "Erro: argumentos invalidos.\n\n"
        );

        trace_generator_print_usage(argv[0]);

        return EXIT_FAILURE;
    }

    if (trace_generator_generate(&options) != 0) {
        fprintf(
            stderr,
            "Erro: nao foi possivel gerar o trace '%s'.\n",
            options.output_path
        );

        return EXIT_FAILURE;
    }

    print_generation_summary(&options);

    return EXIT_SUCCESS;
}