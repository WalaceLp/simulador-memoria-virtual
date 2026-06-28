#include <assert.h>
#include <stdio.h>
#include "page_table.h"

static void test_create_and_destroy(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    page_table_destroy(table);
}

int main(void)
{
    test_create_and_destroy();

    printf("Todos os testes de page_table passaram.\n");
    return 0;
}