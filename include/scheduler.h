#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void addToReadyQueue(int pid);
void refreshReadyQueue(PCB* pcbs, int pcbCount);
void runFCFS(PCB* pcbs, int pcbCount);
void runRR(PCB* pcbs, int pcbCount, int quantum);
void runMLFQ(PCB* pcbs, int pcbCount);

#endif
