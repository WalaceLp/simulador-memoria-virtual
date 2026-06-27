#ifndef CLOCK_H
#define CLOCK_H

#include "frame.h"

/*
 * Estrutura do algoritmo Clock.
 * Mantém apenas a posição atual do
 * ponteiro circular ("hand").
 */
typedef struct {

    int hand;

} Clock;

/* Inicializa o algoritmo Clock */
void clock_init(Clock *clock);

/* Registra o acesso a um frame */
void clock_access(Clock *clock,
                  Frame *frames,
                  int frame_number);

/* Seleciona o próximo frame a ser substituído */
int clock_select_victim(Clock *clock,
                        Frame *frames);

#endif