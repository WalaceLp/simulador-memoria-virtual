#include <stdlib.h>

#include "process.h"

struct Process {
    int pid;
    PageTable *page_table;
};

Process *process_create(int pid)
{
    if (pid < 0) {
        return NULL;
    }

    Process *process = calloc(1, sizeof(Process));

    if (process == NULL) {
        return NULL;
    }

    process->page_table = page_table_create();

    if (process->page_table == NULL) {
        free(process);
        return NULL;
    }

    process->pid = pid;

    return process;
}

int process_get_pid(const Process *process)
{
    if (process == NULL) {
        return -1;
    }

    return process->pid;
}

PageTable *process_get_page_table(Process *process)
{
    if (process == NULL) {
        return NULL;
    }

    return process->page_table;
}

const PageTable *process_get_page_table_const(
    const Process *process
)
{
    if (process == NULL) {
        return NULL;
    }

    return process->page_table;
}

void process_destroy(Process *process)
{
    if (process == NULL) {
        return;
    }

    page_table_destroy(process->page_table);
    free(process);
}