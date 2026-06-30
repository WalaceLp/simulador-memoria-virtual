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
    assert(options.csv_path == NULL);

    assert(options.frame_count == 4);
    assert(options.tlb_entries == 16);
    assert(options.swap_slots == 64);

    assert(options.pid == 1);
    assert(options.policy == REPLACEMENT_FIFO);

    assert(options.remove_swap_on_destroy);
    assert(options.csv_append);
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
    assert(options.tlb_entries == 16);
    assert(options.swap_slots == 64);
    assert(options.pid == 1);
    assert(options.policy == REPLACEMENT_FIFO);
    assert(options.csv_path == NULL);
    assert(options.csv_append);
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
        "--keep-swap",
        "--csv",
        "resultados.csv",
        "--csv-overwrite"
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

    assert(
        strcmp(
            options.swap_path,
            "custom.swap"
        ) == 0
    );

    assert(
        strcmp(
            options.csv_path,
            "resultados.csv"
        ) == 0
    );

    assert(options.frame_count == 8);
    assert(options.tlb_entries == 32);
    assert(options.swap_slots == 128);
    assert(options.pid == 42);

    assert(options.policy == REPLACEMENT_AGING);

    assert(!options.remove_swap_on_destroy);
    assert(!options.csv_append);
    assert(!options.show_help);
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

static void test_csv_append_by_default(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--csv",
        "resultados.csv"
    };

    CliOptions options;

    assert(
        cli_parse(
            5,
            argv,
            &options
        ) == 0
    );

    assert(options.csv_path != NULL);

    assert(
        strcmp(
            options.csv_path,
            "resultados.csv"
        ) == 0
    );

    assert(options.csv_append);
}

static void test_csv_overwrite(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--csv",
        "resultados.csv",
        "--csv-overwrite"
    };

    CliOptions options;

    assert(
        cli_parse(
            6,
            argv,
            &options
        ) == 0
    );

    assert(options.csv_path != NULL);
    assert(!options.csv_append);
}

static void test_keep_swap(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--keep-swap"
    };

    CliOptions options;

    assert(
        cli_parse(
            4,
            argv,
            &options
        ) == 0
    );

    assert(!options.remove_swap_on_destroy);
}

static void test_help_long_option(void)
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

static void test_help_short_option(void)
{
    char *argv[] = {
        "vmsim",
        "-h"
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

static void test_missing_trace_value(void)
{
    char *argv[] = {
        "vmsim",
        "--trace"
    };

    CliOptions options;

    assert(
        cli_parse(
            2,
            argv,
            &options
        ) == -1
    );
}

static void test_missing_csv_value(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--csv"
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

static void test_missing_policy_value(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--policy"
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

static void test_negative_number(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--frames",
        "-1"
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

static void test_zero_frame_count(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--frames",
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

static void test_zero_tlb_entries(void)
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

static void test_zero_swap_slots(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--swap-slots",
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

static void test_invalid_pid(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--pid",
        "-5"
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

static void test_zero_pid_is_valid(void)
{
    char *argv[] = {
        "vmsim",
        "--trace",
        "trace.txt",
        "--pid",
        "0"
    };

    CliOptions options;

    assert(
        cli_parse(
            5,
            argv,
            &options
        ) == 0
    );

    assert(options.pid == 0);
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

static void test_null_arguments(void)
{
    CliOptions options;

    assert(
        cli_parse(
            0,
            NULL,
            &options
        ) == -1
    );

    char *argv[] = {
        "vmsim"
    };

    assert(
        cli_parse(
            1,
            argv,
            NULL
        ) == -1
    );
}

int main(void)
{
    test_default_values();
    test_minimum_valid_arguments();
    test_all_arguments();
    test_each_policy();
    test_csv_append_by_default();
    test_csv_overwrite();
    test_keep_swap();
    test_help_long_option();
    test_help_short_option();
    test_missing_trace();
    test_missing_trace_value();
    test_missing_csv_value();
    test_missing_policy_value();
    test_invalid_number();
    test_negative_number();
    test_zero_frame_count();
    test_zero_tlb_entries();
    test_zero_swap_slots();
    test_invalid_pid();
    test_zero_pid_is_valid();
    test_invalid_policy();
    test_unknown_argument();
    test_null_arguments();

    printf("Todos os testes de cli passaram.\n");

    return 0;
}