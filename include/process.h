#ifndef PROCESS_H
#define PROCESS_H

#include "page_table.h"

typedef struct Process Process;

Process *process_create(int pid);

int process_get_pid(const Process *process);

PageTable *process_get_page_table(Process *process);

const PageTable *process_get_page_table_const(
    const Process *process
);

void process_destroy(Process *process);

#endif