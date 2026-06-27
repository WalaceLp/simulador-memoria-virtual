#ifndef FRAME_H
#define FRAME_H

#include "common.h"

/*
 * Representa um frame da memória física.
 */
typedef struct {

    int frame_number;

    int virtual_page;

    bool occupied;

    bool referenced;

    bool modified;

    unsigned char age;

} Frame;

#endif