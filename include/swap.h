#ifndef SWAP_H
#define SWAP_H

#include <stdio.h>

#include "common.h"

/*
 * Representa o arquivo de swap utilizado
 * pelo simulador.
 */
typedef struct {

    FILE *file;
    char filename[MAX_PATH_LENGTH];

} Swap;

/* Abre (ou cria) o arquivo de swap */
int swap_init(Swap *swap,
              const char *filename);

/* Fecha o arquivo de swap */
void swap_close(Swap *swap);

/* Escreve uma página no arquivo de swap */
int swap_write_page(Swap *swap,
                    int page_number,
                    const void *data);

/* Lê uma página do arquivo de swap */
int swap_read_page(Swap *swap,
                   int page_number,
                   void *data);

#endif