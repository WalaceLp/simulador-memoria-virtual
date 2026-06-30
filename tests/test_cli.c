#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"

static void test_default_values(void)
{
    CliOptions options;

    cli_options_init(&options);

    assert(options.trace_path == NULL);
    assert(options.swap_path != NULL);

    assert(options.frame_count == 4);
    assert(options.tlb_entries == 16);
    assert(options.swap_slots == 64);

    assert(options.pid == 1);
    assert(options.policy == REPLACEMENT_FIFO);

    assert(options.remove_swap_on_destroy);
    assert(!options.show_help);
}

static void test_minimum_valid_arguments(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt"
    };

    CliOptions options;

    assert(
        cli_parse(
            3,
            argv,
            &options
        ) == 0
    );

    assert(
        strcmp(
            options.trace_path,
            "trace.txt"
        ) == 0
    );

    assert(options.frame_count == 4);
    assert(options.policy == REPLACEMENT_FIFO);
}

static void test_all_arguments(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "input.trace",
        "--frames",
        "8",
        "--policy",
        "aging",
        "--tlb",
        "32",
        "--swap",
        "custom.swap",
        "--swap-slots",
        "128",
        "--pid",
        "42",
        "--keep-swap"
    };

    CliOptions options;

    int argc =
        (int)(sizeof(argv) / sizeof(argv[0]));

    assert(
        cli_parse(
            argc,
            argv,
            &options
        ) == 0
    );

    assert(
        strcmp(
            options.trace_path,
            "input.trace"
        ) == 0
    );

    assert(options.frame_count == 8);
    assert(options.tlb_entries == 32);
    assert(options.swap_slots == 128);
    assert(options.pid == 42);

    assert(options.policy == REPLACEMENT_AGING);

    assert(
        strcmp(
            options.swap_path,
            "custom.swap"
        ) == 0
    );

    assert(!options.remove_swap_on_destroy);
}

static void test_each_policy(void)
{
    struct {
        const char *name;
        ReplacementPolicyType expected;
    } cases[] = {
        {"fifo", REPLACEMENT_FIFO},
        {"lru", REPLACEMENT_LRU},
        {"clock", REPLACEMENT_CLOCK},
        {"aging", REPLACEMENT_AGING}
    };

    size_t case_count =
        sizeof(cases) / sizeof(cases[0]);

    for (
        size_t index = 0;
        index < case_count;
        index++
    ) {
        char *argv[] = {
            "vmsim",
            "--trace",
            "trace.txt",
            "--policy",
            (char *)cases[index].name
        };

        CliOptions options;

        assert(
            cli_parse(
                5,
                argv,
                &options
            ) == 0
        );

        assert(
            options.policy ==
            cases[index].expected
        );
    }
}

static void test_help(void)
{
    char *argv[] = {
        "vmsim",
        "--help"
    };

    CliOptions options;

    assert(
        cli_parse(
            2,
            argv,
            &options
        ) == 1
    );

    assert(options.show_help);
}

static void test_missing_trace(void)
{
    char *argv[] = {
        "vmsim",
        "--frames",
        "4"
    };

    CliOptions options;

    assert(
        cli_parse(
            3,
            argv,
            &options
        ) == -1
    );
}

static void test_invalid_number(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--frames",
        "zero"
    };

    CliOptions options;

    assert(
        cli_parse(
            5,
            argv,
            &options
        ) == -1
    );
}

static void test_zero_value(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--tlb",
        "0"
    };

    CliOptions options;

    assert(
        cli_parse(
            5,
            argv,
            &options
        ) == -1
    );
}

static void test_invalid_policy(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--policy",
        "random"
    };

    CliOptions options;

    assert(
        cli_parse(
            5,
            argv,
            &options
        ) == -1
    );
}

static void test_unknown_argument(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--unknown"
    };

    CliOptions options;

    assert(
        cli_parse(
            4,
            argv,
            &options
        ) == -1
    );
}

int main(void)
{
    test_default_values();
    test_minimum_valid_arguments();
    test_all_arguments();
    test_each_policy();
    test_help();
    test_missing_trace();
    test_invalid_number();
    test_zero_value();
    test_invalid_policy();
    test_unknown_argument();

    printf("Todos os testes de cli passaram.\n");

    return 0;
}