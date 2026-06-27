#ifndef LRU_H
#define LRU_H

#include "frame.h"

/*
 * Estrutura do algoritmo LRU.
 * Mantém um contador global utilizado
 * para registrar a ordem dos acessos.
 */
typedef struct {

    unsigned long current_time;

} LRU;

/* Inicializa o algoritmo LRU */
void lru_init(LRU *lru);

/* Registra um acesso a um frame */
void lru_access(LRU *lru,
                Frame *frames,
                int frame_number);

/* Seleciona o frame menos recentemente utilizado */
int lru_select_victim(LRU *lru,
                      Frame *frames);

#endif