#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "process.h"
#include "replacement.h"
#include "trace.h"
#include "virtual_memory.h"

#define SIMPLE_TRACE_PATH "bin/test_simple.trace"
#define LACKEY_TRACE_PATH "bin/test_lackey.trace"

static void create_simple_trace(void)
{
    FILE *file = fopen(SIMPLE_TRACE_PATH, "w");

    assert(file != NULL);

    fprintf(file, "R 0x1000\n");
    fprintf(file, "W 0x2000\n");
    fprintf(file, "R 0x1000\n");

    fclose(file);
}

static void create_lackey_trace(void)
{
    FILE *file = fopen(LACKEY_TRACE_PATH, "w");

    assert(file != NULL);

    fprintf(file, " I 0400000,4\n");
    fprintf(file, " L 0001000,8\n");
    fprintf(file, " S 0002000,4\n");
    fprintf(file, " M 0003000,8\n");

    fclose(file);
}

static void test_simple_trace_reader(void)
{
    create_simple_trace();

    TraceReader *reader =
        trace_reader_open(SIMPLE_TRACE_PATH);

    assert(reader != NULL);

    TraceEntry entry;

    assert(trace_reader_next(reader, &entry) == 1);
    assert(entry.type == TRACE_ACCESS_READ);
    assert(entry.address == 0x1000ULL);

    assert(trace_reader_next(reader, &entry) == 1);
    assert(entry.type == TRACE_ACCESS_WRITE);
    assert(entry.address == 0x2000ULL);

    assert(trace_reader_next(reader, &entry) == 1);
    assert(entry.type == TRACE_ACCESS_READ);

    assert(trace_reader_next(reader, &entry) == 0);

    trace_reader_close(reader);
    remove(SIMPLE_TRACE_PATH);
}

static void test_lackey_modify_becomes_two_accesses(void)
{
    create_lackey_trace();

    TraceReader *reader =
        trace_reader_open(LACKEY_TRACE_PATH);

    assert(reader != NULL);

    TraceEntry entry;

    assert(trace_reader_next(reader, &entry) == 1);
    assert(entry.type == TRACE_ACCESS_READ);
    assert(entry.address == 0x1000ULL);

    assert(trace_reader_next(reader, &entry) == 1);
    assert(entry.type == TRACE_ACCESS_WRITE);
    assert(entry.address == 0x2000ULL);

    assert(trace_reader_next(reader, &entry) == 1);
    assert(entry.type == TRACE_ACCESS_READ);
    assert(entry.address == 0x3000ULL);

    assert(trace_reader_next(reader, &entry) == 1);
    assert(entry.type == TRACE_ACCESS_WRITE);
    assert(entry.address == 0x3000ULL);

    assert(trace_reader_next(reader, &entry) == 0);

    trace_reader_close(reader);
    remove(LACKEY_TRACE_PATH);
}

static void test_trace_replay(void)
{
    create_simple_trace();

    VirtualMemory *memory =
        virtual_memory_create_with_policy_and_tlb(
            2,
            REPLACEMENT_FIFO,
            2
        );

    Process *process = process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    uint64_t processed = 0;

    assert(
        trace_replay(
            memory,
            process,
            SIMPLE_TRACE_PATH,
            &processed
        ) == 0
    );

    assert(processed == 3);

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->total_accesses == 3);
    assert(stats->read_accesses == 2);
    assert(stats->write_accesses == 1);
    assert(stats->page_faults == 2);

    process_destroy(process);
    virtual_memory_destroy(memory);

    remove(SIMPLE_TRACE_PATH);
}

int main(void)
{
    test_simple_trace_reader();
    test_lackey_modify_becomes_two_accesses();
    test_trace_replay();

    printf("Todos os testes de trace passaram.\n");

    return 0;
}