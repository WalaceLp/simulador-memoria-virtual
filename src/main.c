#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "csv_export.h"
#include "process.h"
#include "trace.h"
#include "virtual_memory.h"

static void print_simulation_configuration(
    const CliOptions *options,
    const VirtualMemory *memory
)
{
    printf("=== Configuracao da simulacao ===\n");
    printf("Trace: %s\n", options->trace_path);
    printf("PID: %d\n", options->pid);
    printf("Quadros fisicos: %zu\n", options->frame_count);
    printf("Politica: %s\n", virtual_memory_policy_name(memory));
    printf("Entradas da TLB: %zu\n", options->tlb_entries);
    printf("Slots de swap: %zu\n", options->swap_slots);
    printf("Arquivo de swap: %s\n", options->swap_path);

    if (options->csv_path != NULL) {
        printf("Arquivo CSV: %s\n", options->csv_path);

        printf(
            "Modo do CSV: %s\n",
            options->csv_append
                ? "acrescentar"
                : "sobrescrever"
        );
    }
}

static void print_simulation_results(
    const VirtualMemory *memory,
    uint64_t processed_accesses
)
{
    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    if (stats == NULL) {
        return;
    }

    printf("\n=== Resultados ===\n");

    printf(
        "Acessos processados: %" PRIu64 "\n",
        processed_accesses
    );

    printf(
        "Acessos totais: %" PRIu64 "\n",
        stats->total_accesses
    );

    printf(
        "Leituras: %" PRIu64 "\n",
        stats->read_accesses
    );

    printf(
        "Escritas: %" PRIu64 "\n",
        stats->write_accesses
    );

    printf(
        "Page faults: %" PRIu64 "\n",
        stats->page_faults
    );

    printf(
        "Substituicoes: %" PRIu64 "\n",
        stats->replacements
    );

    printf(
        "Vitimas sujas: %" PRIu64 "\n",
        stats->dirty_evictions
    );

    printf(
        "TLB hits: %" PRIu64 "\n",
        stats->tlb_hits
    );

    printf(
        "TLB misses: %" PRIu64 "\n",
        stats->tlb_misses
    );

    printf(
        "Taxa de acerto da TLB: %.2f%%\n",
        virtual_memory_tlb_hit_ratio(memory) * 100.0
    );

    printf(
        "Page walks: %" PRIu64 "\n",
        stats->page_walks
    );

    printf(
        "Niveis percorridos: %" PRIu64 "\n",
        stats->page_walk_levels
    );

    printf(
        "Media de niveis por page walk: %.2f\n",
        virtual_memory_average_page_walk_levels(memory)
    );

    printf(
        "Leituras do swap: %" PRIu64 "\n",
        stats->swap_reads
    );

    printf(
        "Escritas no swap: %" PRIu64 "\n",
        stats->swap_writes
    );
}

int main(int argc, char **argv)
{
    CliOptions options;

    int parse_result = cli_parse(
        argc,
        argv,
        &options
    );

    if (parse_result == 1) {
        cli_print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (parse_result != 0) {
        fprintf(
            stderr,
            "Erro: argumentos invalidos.\n\n"
        );

        cli_print_usage(argv[0]);

        return EXIT_FAILURE;
    }

    VirtualMemory *memory =
        virtual_memory_create_complete(
            options.frame_count,
            options.policy,
            options.tlb_entries,
            options.swap_path,
            options.swap_slots,
            options.remove_swap_on_destroy
        );

    if (memory == NULL) {
        fprintf(
            stderr,
            "Erro: nao foi possivel criar a memoria virtual.\n"
        );

        return EXIT_FAILURE;
    }

    Process *process =
        process_create(options.pid);

    if (process == NULL) {
        fprintf(
            stderr,
            "Erro: nao foi possivel criar o processo.\n"
        );

        virtual_memory_destroy(memory);

        return EXIT_FAILURE;
    }

    print_simulation_configuration(
        &options,
        memory
    );

    uint64_t processed_accesses = 0;

    int replay_result = trace_replay(
        memory,
        process,
        options.trace_path,
        &processed_accesses
    );

    if (replay_result != 0) {
        fprintf(
            stderr,
            "Erro: falha ao processar o trace '%s'.\n",
            options.trace_path
        );

        process_destroy(process);
        virtual_memory_destroy(memory);

        return EXIT_FAILURE;
    }

    print_simulation_results(
        memory,
        processed_accesses
    );

    if (options.csv_path != NULL) {
        int export_result = csv_export_result(
            options.csv_path,
            &options,
            memory,
            processed_accesses,
            options.csv_append
        );

        if (export_result != 0) {
            fprintf(
                stderr,
                "Erro: nao foi possivel exportar o CSV '%s'.\n",
                options.csv_path
            );

            process_destroy(process);
            virtual_memory_destroy(memory);

            return EXIT_FAILURE;
        }

        printf(
            "\nResultados exportados para: %s\n",
            options.csv_path
        );
    }

    process_destroy(process);
    virtual_memory_destroy(memory);

    return EXIT_SUCCESS;
}