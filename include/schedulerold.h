// #ifndef SCHEDULER_H
// #define SCHEDULER_H

// #include "process.h"

// void checkArrivals();
// void addToReadyQueue(int pid);
// // void refreshReadyQueue(PCB* pcbs, int pcbCount);
// // void runFCFS(PCB* pcbs, int pcbCount);
// void runFCFS();
// // void runRR(PCB* pcbs, int pcbCount, int quantum);
// void runRR(int quantum);
// // void runMLFQ(PCB* pcbs, int pcbCount);
// void runMLFQ();

// #endif
// scheduler.h

#ifndef SCHEDULEROLD_H
#define SCHEDULEROLD_H

#include "process.h"

void addToReadyQueue(int pid);
int getNextProcessFCFS();
int getNextProcessRR(int quantum);
void addToMLFQ(int pid, int level);
int getNextProcessMLFQ();

void initScheduler();
void promoteUnblockedProcesses();
void resetScheduler();

extern int schedulerType; // 1 = FCFS, 2 = RR, 3 = MLFQ
extern int rrQuantum;
extern int quantumUsed[];

#endif