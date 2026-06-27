#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

/* ==========================
   Configurações do Simulador
   ========================== */

#define PAGE_SIZE      4096
#define FRAME_COUNT    64
#define TLB_SIZE       16
#define TRIE_LEVELS    4
#define MAX_PATH_LENGTH  256
/* ==========================
   Tipos de Acesso
   ========================== */

typedef enum {
    READ,
    WRITE
} AccessType;

/* ==========================
   Algoritmos de Substituição
   ========================== */

typedef enum {
    FIFO_POLICY,
    LRU_POLICY,
    CLOCK_POLICY,
    AGING_POLICY 
} ReplacementPolicyType;

/* ==========================
   Resultado da Tradução
   ========================== */

typedef enum {
    TRANSLATION_OK,
    PAGE_FAULT
} TranslationStatus;

#endif