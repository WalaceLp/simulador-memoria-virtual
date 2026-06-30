#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "csv_export.h"
#include "process.h"
#include "replacement.h"
#include "virtual_memory.h"

#define TEST_CSV_PATH "bin/test_metrics.csv"

static VirtualMemory *create_test_simulation(
    Process **process
)
{
    VirtualMemory *memory =
        virtual_memory_create_with_policy_and_tlb(
            2,
            REPLACEMENT_LRU,
            2
        );

    assert(memory != NULL);

    *process = process_create(7);

    assert(*process != NULL);

    assert(
        virtual_memory_access(
            memory,
            *process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            *process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_OK
    );

    assert(
        virtual_memory_access(
            memory,
            *process,
            0x2000ULL,
            VM_ACCESS_WRITE
        ) == VM_ACCESS_PAGE_FAULT
    );

    return memory;
}

static CliOptions create_test_options(void)
{
    CliOptions options;

    cli_options_init(&options);

    options.trace_path = "traces/teste,com-virgula.trace";
    options.frame_count = 2;
    options.tlb_entries = 2;
    options.swap_slots = 8;
    options.pid = 7;
    options.policy = REPLACEMENT_LRU;
    options.csv_path = TEST_CSV_PATH;

    return options;
}

static void test_export_creates_header_and_row(void)
{
    Process *process = NULL;

    VirtualMemory *memory =
        create_test_simulation(&process);

    CliOptions options =
        create_test_options();

    assert(
        csv_export_result(
            TEST_CSV_PATH,
            &options,
            memory,
            3,
            false
        ) == 0
    );

    FILE *file = fopen(TEST_CSV_PATH, "r");

    assert(file != NULL);

    char header[1024];
    char row[2048];

    assert(fgets(header, sizeof(header), file) != NULL);
    assert(fgets(row, sizeof(row), file) != NULL);
    assert(fgets(header, sizeof(header), file) == NULL);

    assert(strstr(header, "page_faults") != NULL);
    assert(strstr(header, "tlb_hit_ratio") != NULL);
    assert(strstr(header, "swap_writes") != NULL);

    assert(
        strstr(
            row,
            "\"traces/teste,com-virgula.trace\""
        ) != NULL
    );

    assert(strstr(row, "\"LRU\"") != NULL);
    assert(strstr(row, ",7,2,") != NULL);

    fclose(file);

    process_destroy(process);
    virtual_memory_destroy(memory);

    remove(TEST_CSV_PATH);
}

static void test_append_does_not_repeat_header(void)
{
    Process *process = NULL;

    VirtualMemory *memory =
        create_test_simulation(&process);

    CliOptions options =
        create_test_options();

    assert(
        csv_export_result(
            TEST_CSV_PATH,
            &options,
            memory,
            3,
            false
        ) == 0
    );

    assert(
        csv_export_result(
            TEST_CSV_PATH,
            &options,
            memory,
            3,
            true
        ) == 0
    );

    FILE *file = fopen(TEST_CSV_PATH, "r");

    assert(file != NULL);

    char line[2048];
    size_t line_count = 0;
    size_t header_count = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line_count++;

        if (strstr(line, "page_faults") != NULL) {
            header_count++;
        }
    }

    assert(line_count == 3);
    assert(header_count == 1);

    fclose(file);

    process_destroy(process);
    virtual_memory_destroy(memory);

    remove(TEST_CSV_PATH);
}

static void test_overwrite_replaces_previous_content(void)
{
    Process *process = NULL;

    VirtualMemory *memory =
        create_test_simulation(&process);

    CliOptions options =
        create_test_options();

    assert(
        csv_export_result(
            TEST_CSV_PATH,
            &options,
            memory,
            3,
            false
        ) == 0
    );

    assert(
        csv_export_result(
            TEST_CSV_PATH,
            &options,
            memory,
            10,
            false
        ) == 0
    );

    FILE *file = fopen(TEST_CSV_PATH, "r");

    assert(file != NULL);

    char line[2048];
    size_t line_count = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line_count++;
    }

    assert(line_count == 2);

    fclose(file);

    process_destroy(process);
    virtual_memory_destroy(memory);

    remove(TEST_CSV_PATH);
}

static void test_invalid_arguments(void)
{
    CliOptions options =
        create_test_options();

    assert(
        csv_export_result(
            NULL,
            &options,
            NULL,
            0,
            true
        ) == -1
    );

    assert(
        csv_export_result(
            "",
            &options,
            NULL,
            0,
            true
        ) == -1
    );
}

int main(void)
{
    test_export_creates_header_and_row();
    test_append_does_not_repeat_header();
    test_overwrite_replaces_previous_content();
    test_invalid_arguments();

    printf(
        "Todos os testes de csv_export passaram.\n"
    );

    return 0;
}