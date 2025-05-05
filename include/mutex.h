#ifndef MUTEX_H
#define MUTEX_H

#include "process.h"

#define MAX_QUEUE 5

extern int userInput, userOutput, file;
extern int userInputQueue[MAX_QUEUE];
extern int userOutputQueue[MAX_QUEUE];
extern int fileQueue[MAX_QUEUE];
extern int inputCount, outputCount, fileCount;

void initMutexes();

int semWait(PCB* pcb, const char* resource);
void semSignal(const char* resource);

#endif
