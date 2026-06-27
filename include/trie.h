#ifndef TRIE_H
#define TRIE_H

#include "common.h"

/*
 * Número de filhos por nó da Trie.
 * Valor reduzido para fins didáticos.
 */
#define TRIE_CHILDREN 16

/*
 * Estrutura de um nó da Trie.
 */
typedef struct TrieNode {

    struct TrieNode *children[TRIE_CHILDREN];

    bool valid;

    int frame_number;

} TrieNode;

/*
 * Estrutura da Trie.
 */
typedef struct {

    TrieNode *root;

} Trie;

/* Cria uma Trie vazia */
Trie *trie_create(void);

/* Insere uma página virtual */
bool trie_insert(Trie *trie, int virtual_page, int frame);

/* Busca uma página virtual */
bool trie_search(Trie *trie, int virtual_page, int *frame);

/* Remove uma página virtual */
bool trie_remove(Trie *trie, int virtual_page);

/* Libera toda a Trie */
void trie_destroy(Trie *trie);

#endif