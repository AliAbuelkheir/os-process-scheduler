#ifndef MUTEX_H
#define MUTEX_H

#include "process.h"

void initMutexes();

int semWait(PCB* pcb, const char* resource);
void semSignal(const char* resource);

#endif
