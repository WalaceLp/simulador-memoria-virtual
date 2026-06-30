#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "csv_export.h"

static bool csv_file_is_empty(FILE *file)
{
    if (file == NULL) {
        return true;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        return true;
    }

    long size = ftell(file);

    return size <= 0;
}

/*
 * Escreve um campo textual entre aspas e duplica
 * aspas internas conforme o formato CSV.
 */
static int csv_write_text_field(
    FILE *file,
    const char *text
)
{
    if (file == NULL) {
        return -1;
    }

    if (text == NULL) {
        text = "";
    }

    if (fputc('"', file) == EOF) {
        return -1;
    }

    for (
        const char *cursor = text;
        *cursor != '\0';
        cursor++
    ) {
        if (*cursor == '"') {
            if (
                fputc('"', file) == EOF ||
                fputc('"', file) == EOF
            ) {
                return -1;
            }

            continue;
        }

        if (fputc(*cursor, file) == EOF) {
            return -1;
        }
    }

    if (fputc('"', file) == EOF) {
        return -1;
    }

    return 0;
}

static int csv_write_header(FILE *file)
{
    if (file == NULL) {
        return -1;
    }

    int result = fprintf(
        file,
        "trace,"
        "pid,"
        "frames,"
        "policy,"
        "tlb_entries,"
        "swap_slots,"
        "processed_accesses,"
        "total_accesses,"
        "read_accesses,"
        "write_accesses,"
        "page_faults,"
        "replacements,"
        "dirty_evictions,"
        "tlb_hits,"
        "tlb_misses,"
        "tlb_hit_ratio,"
        "page_walks,"
        "page_walk_levels,"
        "average_page_walk_levels,"
        "swap_reads,"
        "swap_writes\n"
    );

    return result < 0 ? -1 : 0;
}

static int csv_write_result_row(
    FILE *file,
    const CliOptions *options,
    const VirtualMemory *memory,
    uint64_t processed_accesses
)
{
    if (
        file == NULL ||
        options == NULL ||
        memory == NULL
    ) {
        return -1;
    }

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    if (stats == NULL) {
        return -1;
    }

    if (
        csv_write_text_field(
            file,
            options->trace_path
        ) != 0
    ) {
        return -1;
    }

    if (fputc(',', file) == EOF) {
        return -1;
    }

    int result = fprintf(
        file,
        "%d,"
        "%zu,",
        options->pid,
        options->frame_count
    );

    if (result < 0) {
        return -1;
    }

    if (
        csv_write_text_field(
            file,
            virtual_memory_policy_name(memory)
        ) != 0
    ) {
        return -1;
    }

    if (fputc(',', file) == EOF) {
        return -1;
    }

    result = fprintf(
        file,
        "%zu,"
        "%zu,"
        "%" PRIu64 ","
        "%" PRIu64 ","
        "%" PRIu64 ","
        "%" PRIu64 ","
        "%" PRIu64 ","
        "%" PRIu64 ","
        "%" PRIu64 ","
        "%" PRIu64 ","
        "%" PRIu64 ","
        "%.6f,"
        "%" PRIu64 ","
        "%" PRIu64 ","
        "%.6f,"
        "%" PRIu64 ","
        "%" PRIu64 "\n",
        options->tlb_entries,
        options->swap_slots,
        processed_accesses,
        stats->total_accesses,
        stats->read_accesses,
        stats->write_accesses,
        stats->page_faults,
        stats->replacements,
        stats->dirty_evictions,
        stats->tlb_hits,
        stats->tlb_misses,
        virtual_memory_tlb_hit_ratio(memory),
        stats->page_walks,
        stats->page_walk_levels,
        virtual_memory_average_page_walk_levels(memory),
        stats->swap_reads,
        stats->swap_writes
    );

    return result < 0 ? -1 : 0;
}

int csv_export_result(
    const char *path,
    const CliOptions *options,
    const VirtualMemory *memory,
    uint64_t processed_accesses,
    bool append
)
{
    if (
        path == NULL ||
        *path == '\0' ||
        options == NULL ||
        memory == NULL
    ) {
        return -1;
    }

    const char *mode = append ? "a+" : "w";

    FILE *file = fopen(path, mode);

    if (file == NULL) {
        return -1;
    }

    bool write_header =
        !append || csv_file_is_empty(file);

    if (write_header && csv_write_header(file) != 0) {
        fclose(file);
        return -1;
    }

    if (
        csv_write_result_row(
            file,
            options,
            memory,
            processed_accesses
        ) != 0
    ) {
        fclose(file);
        return -1;
    }

    if (fflush(file) != 0) {
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        return -1;
    }

    return 0;
}