#include <assert.h>
#include <stdio.h>

#include "replacement.h"

static void test_invalid_policy_creation(void)
{
    assert(
        replacement_policy_create(
            REPLACEMENT_FIFO,
            0
        ) == NULL
    );

    assert(
        replacement_policy_create(
            (ReplacementPolicyType)99,
            4
        ) == NULL
    );
}

static void test_fifo(void)
{
    ReplacementPolicy *policy =
        replacement_policy_create(
            REPLACEMENT_FIFO,
            4
        );

    assert(policy != NULL);

    assert(replacement_policy_on_load(policy, 0) == 0);
    assert(replacement_policy_on_load(policy, 1) == 0);
    assert(replacement_policy_on_load(policy, 2) == 0);

    assert(
        replacement_policy_choose_victim(policy) == 0
    );

    assert(replacement_policy_on_evict(policy, 0) == 0);
    assert(replacement_policy_on_load(policy, 3) == 0);

    assert(
        replacement_policy_choose_victim(policy) == 1
    );

    replacement_policy_destroy(policy);
}

static void test_lru(void)
{
    ReplacementPolicy *policy =
        replacement_policy_create(
            REPLACEMENT_LRU,
            3
        );

    assert(policy != NULL);

    assert(replacement_policy_on_load(policy, 0) == 0);
    assert(replacement_policy_on_load(policy, 1) == 0);
    assert(replacement_policy_on_load(policy, 2) == 0);

    /*
     * O quadro 0 volta a ser o mais recentemente usado.
     */
    assert(replacement_policy_on_access(policy, 0) == 0);

    /*
     * O quadro 1 passa a ser o menos recentemente usado.
     */
    assert(
        replacement_policy_choose_victim(policy) == 1
    );

    replacement_policy_destroy(policy);
}

static void test_clock(void)
{
    ReplacementPolicy *policy =
        replacement_policy_create(
            REPLACEMENT_CLOCK,
            3
        );

    assert(policy != NULL);

    assert(replacement_policy_on_load(policy, 0) == 0);
    assert(replacement_policy_on_load(policy, 1) == 0);
    assert(replacement_policy_on_load(policy, 2) == 0);

    /*
     * Na primeira volta, todos recebem segunda chance.
     * Depois, o quadro 0 é escolhido.
     */
    assert(
        replacement_policy_choose_victim(policy) == 0
    );

    assert(replacement_policy_on_evict(policy, 0) == 0);

    /*
     * O quadro 1 é referenciado novamente.
     */
    assert(replacement_policy_on_access(policy, 1) == 0);

    /*
     * O quadro 2 não recebeu uma nova referência.
     */
    assert(
        replacement_policy_choose_victim(policy) == 2
    );

    replacement_policy_destroy(policy);
}

static void test_aging(void)
{
    ReplacementPolicy *policy =
        replacement_policy_create(
            REPLACEMENT_AGING,
            3
        );

    assert(policy != NULL);

    assert(replacement_policy_on_load(policy, 0) == 0);
    assert(replacement_policy_on_load(policy, 1) == 0);
    assert(replacement_policy_on_load(policy, 2) == 0);

    replacement_policy_tick(policy);

    assert(replacement_policy_on_access(policy, 1) == 0);

    replacement_policy_tick(policy);

    assert(replacement_policy_on_access(policy, 2) == 0);

    replacement_policy_tick(policy);

    /*
     * O quadro 0 não recebeu novas referências e
     * acumulou o menor contador.
     */
    assert(
        replacement_policy_choose_victim(policy) == 0
    );

    replacement_policy_destroy(policy);
}

static void test_invalid_operations(void)
{
    ReplacementPolicy *policy =
        replacement_policy_create(
            REPLACEMENT_FIFO,
            2
        );

    assert(policy != NULL);

    assert(
        replacement_policy_on_load(policy, 10) == -1
    );

    assert(
        replacement_policy_on_access(policy, 1) == -1
    );

    assert(
        replacement_policy_on_evict(policy, 1) == -1
    );

    replacement_policy_destroy(policy);
    replacement_policy_destroy(NULL);
}

static void test_policy_names(void)
{
    ReplacementPolicy *fifo =
        replacement_policy_create(
            REPLACEMENT_FIFO,
            1
        );

    ReplacementPolicy *lru =
        replacement_policy_create(
            REPLACEMENT_LRU,
            1
        );

    ReplacementPolicy *clock =
        replacement_policy_create(
            REPLACEMENT_CLOCK,
            1
        );

    ReplacementPolicy *aging =
        replacement_policy_create(
            REPLACEMENT_AGING,
            1
        );

    assert(fifo != NULL);
    assert(lru != NULL);
    assert(clock != NULL);
    assert(aging != NULL);

    assert(replacement_policy_name(fifo)[0] == 'F');
    assert(replacement_policy_name(lru)[0] == 'L');
    assert(replacement_policy_name(clock)[0] == 'C');
    assert(replacement_policy_name(aging)[0] == 'A');

    replacement_policy_destroy(fifo);
    replacement_policy_destroy(lru);
    replacement_policy_destroy(clock);
    replacement_policy_destroy(aging);
}

int main(void)
{
    test_invalid_policy_creation();
    test_fifo();
    test_lru();
    test_clock();
    test_aging();
    test_invalid_operations();
    test_policy_names();

    printf(
        "Todos os testes de replacement passaram.\n"
    );

    return 0;
}