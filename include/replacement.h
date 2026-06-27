#ifndef REPLACEMENT_H
#define REPLACEMENT_H

#include "common.h"
#include "frame.h"

/*
 * Representa o algoritmo de substituição
 * atualmente utilizado pelo simulador.
 */
typedef struct {

    ReplacementPolicyType type;

} ReplacementPolicy;

/* Inicializa o algoritmo escolhido */
void replacement_init(ReplacementPolicy *policy,
                      ReplacementPolicyType type);

/* Informa que uma página foi acessada */
void replacement_access(ReplacementPolicy *policy,
                        Frame frames[],
                        int frame_number);

/* Escolhe qual frame será substituído */
int replacement_select_victim(ReplacementPolicy *policy,
                              Frame frames[]);

#endif