#ifndef FIFO_H
#define FIFO_H

// #include "frame.h"

/*
 * Estrutura do algoritmo FIFO.
 * Mantém apenas o índice do próximo
 * frame que deverá ser substituído.
 */
typedef struct {

    int next;

} FIFO;

/* Inicializa o algoritmo FIFO */
void fifo_init(FIFO *fifo);

/* Informa que um frame foi acessado.
 * No FIFO essa operação não altera o estado.
 */
void fifo_access(FIFO *fifo,
                 int frame_number);

/* Seleciona o próximo frame a ser substituído */
int fifo_select_victim(FIFO *fifo);

#endif