#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "address.h"
#include "trace.h"

#define TRACE_LINE_SIZE 256

struct TraceReader {
    FILE *file;

    bool has_pending_entry;
    TraceEntry pending_entry;
};

TraceReader *trace_reader_open(const char *path)
{
    if (path == NULL) {
        return NULL;
    }

    TraceReader *reader = calloc(
        1,
        sizeof(TraceReader)
    );

    if (reader == NULL) {
        return NULL;
    }

    reader->file = fopen(path, "r");

    if (reader->file == NULL) {
        free(reader);
        return NULL;
    }

    return reader;
}

void trace_reader_close(TraceReader *reader)
{
    if (reader == NULL) {
        return;
    }

    if (reader->file != NULL) {
        fclose(reader->file);
    }

    free(reader);
}

static char *trace_skip_whitespace(char *text)
{
    while (
        text != NULL &&
        *text != '\0' &&
        isspace((unsigned char)*text)
    ) {
        text++;
    }

    return text;
}

static int trace_parse_parameters(
    const char *text,
    uint64_t *address,
    size_t *size
)
{
    if (
        text == NULL ||
        address == NULL ||
        size == NULL
    ) {
        return -1;
    }

    unsigned long long parsed_address = 0;
    size_t parsed_size = 1;

    int matched = sscanf(
        text,
        " %llx,%zu",
        &parsed_address,
        &parsed_size
    );

    if (matched < 1) {
        matched = sscanf(
            text,
            " %llx %zu",
            &parsed_address,
            &parsed_size
        );
    }

    if (matched < 1 || parsed_size == 0) {
        return -1;
    }

    *address = (uint64_t)parsed_address;
    *size = parsed_size;

    return 0;
}

static int trace_parse_line(
    TraceReader *reader,
    char *line,
    TraceEntry *entry
)
{
    char *cursor = trace_skip_whitespace(line);

    if (
        cursor == NULL ||
        *cursor == '\0' ||
        *cursor == '\n' ||
        *cursor == '#'
    ) {
        return 0;
    }

    char operation = (char)toupper(
        (unsigned char)*cursor
    );

    /*
     * Instruções buscadas pela CPU são ignoradas.
     * O simulador trata somente acessos a dados.
     */
    if (operation == 'I') {
        return 0;
    }

    if (
        operation != 'R' &&
        operation != 'W' &&
        operation != 'L' &&
        operation != 'S' &&
        operation != 'M'
    ) {
        return -1;
    }

    uint64_t address = 0;
    size_t size = 1;

    if (
        trace_parse_parameters(
            cursor + 1,
            &address,
            &size
        ) != 0
    ) {
        return -1;
    }

    entry->address = address;
    entry->size = size;

    if (operation == 'R' || operation == 'L') {
        entry->type = TRACE_ACCESS_READ;
        return 1;
    }

    if (operation == 'W' || operation == 'S') {
        entry->type = TRACE_ACCESS_WRITE;
        return 1;
    }

    /*
     * M, no Lackey, representa leitura seguida de escrita.
     */
    entry->type = TRACE_ACCESS_READ;

    reader->pending_entry.type =
        TRACE_ACCESS_WRITE;

    reader->pending_entry.address = address;
    reader->pending_entry.size = size;
    reader->has_pending_entry = true;

    return 1;
}

int trace_reader_next(
    TraceReader *reader,
    TraceEntry *entry
)
{
    if (
        reader == NULL ||
        reader->file == NULL ||
        entry == NULL
    ) {
        return -1;
    }

    if (reader->has_pending_entry) {
        *entry = reader->pending_entry;
        reader->has_pending_entry = false;

        return 1;
    }

    char line[TRACE_LINE_SIZE];

    while (fgets(line, sizeof(line), reader->file) != NULL) {
        int result = trace_parse_line(
            reader,
            line,
            entry
        );

        if (result != 0) {
            return result;
        }
    }

    if (ferror(reader->file)) {
        return -1;
    }

    return 0;
}

static int trace_replay_entry(
    VirtualMemory *memory,
    Process *process,
    const TraceEntry *entry,
    uint64_t *processed_accesses
)
{
    if (
        memory == NULL ||
        process == NULL ||
        entry == NULL ||
        processed_accesses == NULL ||
        entry->size == 0
    ) {
        return -1;
    }

    uint64_t size_minus_one =
        (uint64_t)(entry->size - 1);

    if (size_minus_one > UINT64_MAX - entry->address) {
        return -1;
    }

    uint64_t final_address =
        entry->address + size_minus_one;

    uint64_t first_page =
        address_page_number(entry->address);

    uint64_t last_page =
        address_page_number(final_address);

    VirtualMemoryAccessType access_type =
        entry->type == TRACE_ACCESS_WRITE
            ? VM_ACCESS_WRITE
            : VM_ACCESS_READ;

    for (
        uint64_t page = first_page;
        page <= last_page;
        page++
    ) {
        uint64_t address =
            page == first_page
                ? entry->address
                : page << PAGE_OFFSET_BITS;

        VirtualMemoryAccessResult result =
            virtual_memory_access(
                memory,
                process,
                address,
                access_type
            );

        if (result < 0) {
            return -1;
        }

        (*processed_accesses)++;

        if (page == UINT64_MAX) {
            break;
        }
    }

    return 0;
}

int trace_replay(
    VirtualMemory *memory,
    Process *process,
    const char *path,
    uint64_t *processed_accesses
)
{
    if (
        memory == NULL ||
        process == NULL ||
        path == NULL ||
        processed_accesses == NULL
    ) {
        return -1;
    }

    TraceReader *reader =
        trace_reader_open(path);

    if (reader == NULL) {
        return -1;
    }

    *processed_accesses = 0;

    TraceEntry entry;

    int result = 0;

    while (
        (result = trace_reader_next(
            reader,
            &entry
        )) == 1
    ) {
        if (
            trace_replay_entry(
                memory,
                process,
                &entry,
                processed_accesses
            ) != 0
        ) {
            trace_reader_close(reader);
            return -1;
        }
    }

    trace_reader_close(reader);

    return result == 0 ? 0 : -1;
}