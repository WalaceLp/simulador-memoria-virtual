#include <stdio.h>

int main(void)
{
    test_create_and_destroy();
    test_lookup_unmapped_address();
    test_map_and_lookup();
    test_addresses_inside_same_page();
    test_multiple_mappings();
    test_remap_existing_page();

    test_unmap_existing_page();
    test_unmap_unmapped_page();
    test_unmap_preserves_other_page();
    test_unmap_prunes_empty_nodes();
    test_remap_after_unmap();
    test_unmap_address_inside_same_page();

    printf("Todos os testes de page_table passaram.\n");

    return 0;
}