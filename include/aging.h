#ifndef AGING_H
#define AGING_H

#include "frame.h"

/*
 * Estrutura do algoritmo Aging.
 * Não necessita armazenar estado próprio,
 * pois as informações ficam nos frames.
 */
typedef struct {

    int unused;

} Aging;

/* Inicializa o algoritmo Aging */
void aging_init(Aging *aging);

/* Registra o acesso a um frame */
void aging_access(Aging *aging,
                  Frame *frames,
                  int frame_number);

/* Atualiza os contadores de envelhecimento */
void aging_update(Aging *aging,
                  Frame *frames);

/* Seleciona o frame com menor contador */
int aging_select_victim(Aging *aging,
                        Frame *frames);

#endif