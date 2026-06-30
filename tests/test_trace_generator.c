#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "address.h"
#include "trace_generator.h"

#define SEQUENTIAL_PATH \
    "bin/test_sequential_generated.trace"

#define RANDOM_FIRST_PATH \
    "bin/test_random_first.trace"

#define RANDOM_SECOND_PATH \
    "bin/test_random_second.trace"

static size_t count_file_lines(const char *path)
{
    FILE *file = fopen(path, "r");

    assert(file != NULL);

    size_t line_count = 0;
    char line[128];

    while (fgets(line, sizeof(line), file) != NULL) {
        line_count++;
    }

    fclose(file);

    return line_count;
}

static void test_default_options(void)
{
    TraceGeneratorOptions options;

    trace_generator_options_init(&options);

    assert(options.output_path == NULL);
    assert(
        options.pattern ==
        TRACE_PATTERN_SEQUENTIAL
    );

    assert(options.page_count == 16);
    assert(options.access_count == 1000);
    assert(options.seed == 1);
    assert(options.base_address == 0x1000ULL);
    assert(options.write_percentage == 20);
    assert(!options.show_help);
}

static void test_parse_all_arguments(void)
{
    char *argv[] = {
        "tracegen",
        "--output",
        "trace.out",
        "--pattern",
        "random",
        "--pages",
        "32",
        "--accesses",
        "500",
        "--seed",
        "42",
        "--base",
        "0x4000",
        "--write-rate",
        "35"
    };

    int argc =
        (int)(sizeof(argv) / sizeof(argv[0]));

    TraceGeneratorOptions options;

    assert(
        trace_generator_parse(
            argc,
            argv,
            &options
        ) == 0
    );

    assert(
        strcmp(
            options.output_path,
            "trace.out"
        ) == 0
    );

    assert(options.pattern == TRACE_PATTERN_RANDOM);
    assert(options.page_count == 32);
    assert(options.access_count == 500);
    assert(options.seed == 42);
    assert(options.base_address == 0x4000ULL);
    assert(options.write_percentage == 35);
}

static void test_missing_output(void)
{
    char *argv[] = {
        "tracegen",
        "--pages",
        "10"
    };

    TraceGeneratorOptions options;

    assert(
        trace_generator_parse(
            3,
            argv,
            &options
        ) == -1
    );
}

static void test_invalid_pattern(void)
{
    char *argv[] = {
        "tracegen",
        "--output",
        "trace.out",
        "--pattern",
        "reverse"
    };

    TraceGeneratorOptions options;

    assert(
        trace_generator_parse(
            5,
            argv,
            &options
        ) == -1
    );
}

static void test_invalid_write_rate(void)
{
    char *argv[] = {
        "tracegen",
        "--output",
        "trace.out",
        "--write-rate",
        "101"
    };

    TraceGeneratorOptions options;

    assert(
        trace_generator_parse(
            5,
            argv,
            &options
        ) == -1
    );
}

static void test_help(void)
{
    char *argv[] = {
        "tracegen",
        "--help"
    };

    TraceGeneratorOptions options;

    assert(
        trace_generator_parse(
            2,
            argv,
            &options
        ) == 1
    );

    assert(options.show_help);
}

static void test_sequential_generation(void)
{
    TraceGeneratorOptions options;

    trace_generator_options_init(&options);

    options.output_path = SEQUENTIAL_PATH;
    options.pattern = TRACE_PATTERN_SEQUENTIAL;
    options.page_count = 3;
    options.access_count = 5;
    options.base_address = 0x1000ULL;
    options.write_percentage = 0;
    options.seed = 7;

    assert(trace_generator_generate(&options) == 0);
    assert(count_file_lines(SEQUENTIAL_PATH) == 5);

    FILE *file = fopen(SEQUENTIAL_PATH, "r");

    assert(file != NULL);

    char operation = '\0';
    unsigned long long address = 0;

    assert(
        fscanf(
            file,
            " %c 0x%llx",
            &operation,
            &address
        ) == 2
    );

    assert(operation == 'R');
    assert(address == 0x1000ULL);

    assert(
        fscanf(
            file,
            " %c 0x%llx",
            &operation,
            &address
        ) == 2
    );

    assert(address == 0x1000ULL + PAGE_SIZE);

    assert(
        fscanf(
            file,
            " %c 0x%llx",
            &operation,
            &address
        ) == 2
    );

    assert(
        address ==
        0x1000ULL + 2ULL * PAGE_SIZE
    );

    assert(
        fscanf(
            file,
            " %c 0x%llx",
            &operation,
            &address
        ) == 2
    );

    assert(address == 0x1000ULL);

    fclose(file);
    remove(SEQUENTIAL_PATH);
}

static void test_random_generation_is_reproducible(void)
{
    TraceGeneratorOptions options;

    trace_generator_options_init(&options);

    options.pattern = TRACE_PATTERN_RANDOM;
    options.page_count = 8;
    options.access_count = 50;
    options.write_percentage = 30;
    options.seed = 12345;

    options.output_path = RANDOM_FIRST_PATH;

    assert(trace_generator_generate(&options) == 0);

    options.output_path = RANDOM_SECOND_PATH;

    assert(trace_generator_generate(&options) == 0);

    FILE *first = fopen(RANDOM_FIRST_PATH, "r");
    FILE *second = fopen(RANDOM_SECOND_PATH, "r");

    assert(first != NULL);
    assert(second != NULL);

    char first_line[128];
    char second_line[128];

    while (
        fgets(
            first_line,
            sizeof(first_line),
            first
        ) != NULL
    ) {
        assert(
            fgets(
                second_line,
                sizeof(second_line),
                second
            ) != NULL
        );

        assert(strcmp(first_line, second_line) == 0);
    }

    assert(
        fgets(
            second_line,
            sizeof(second_line),
            second
        ) == NULL
    );

    fclose(first);
    fclose(second);

    remove(RANDOM_FIRST_PATH);
    remove(RANDOM_SECOND_PATH);
}

static void test_all_reads(void)
{
    TraceGeneratorOptions options;

    trace_generator_options_init(&options);

    options.output_path = SEQUENTIAL_PATH;
    options.page_count = 2;
    options.access_count = 20;
    options.write_percentage = 0;

    assert(trace_generator_generate(&options) == 0);

    FILE *file = fopen(SEQUENTIAL_PATH, "r");

    assert(file != NULL);

    char line[128];

    while (fgets(line, sizeof(line), file) != NULL) {
        assert(line[0] == 'R');
    }

    fclose(file);
    remove(SEQUENTIAL_PATH);
}

static void test_all_writes(void)
{
    TraceGeneratorOptions options;

    trace_generator_options_init(&options);

    options.output_path = SEQUENTIAL_PATH;
    options.page_count = 2;
    options.access_count = 20;
    options.write_percentage = 100;

    assert(trace_generator_generate(&options) == 0);

    FILE *file = fopen(SEQUENTIAL_PATH, "r");

    assert(file != NULL);

    char line[128];

    while (fgets(line, sizeof(line), file) != NULL) {
        assert(line[0] == 'W');
    }

    fclose(file);
    remove(SEQUENTIAL_PATH);
}

int main(void)
{
    test_default_options();
    test_parse_all_arguments();
    test_missing_output();
    test_invalid_pattern();
    test_invalid_write_rate();
    test_help();
    test_sequential_generation();
    test_random_generation_is_reproducible();
    test_all_reads();
    test_all_writes();

    printf(
        "Todos os testes de trace_generator passaram.\n"
    );

    return 0;
}