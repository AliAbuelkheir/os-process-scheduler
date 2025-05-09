#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "process.h" 
#include <pthread.h>  

void initScheduler(int type, int quantum);
void addToReadyQueue(int pid);
void addToMLFQ(int pid, int level);
void startScheduler();
int getFinishedProcessCount();
int getTotalProcessCount();
int getCurrentClockCycle();
int getCurrentRunningProcess();
void resetScheduler();
void loadArrivedProcesses();
void runFCFS();
void tickFCFS();
void runRR();
void tickRR();
void runMLFQ();
void tickMLFQ();
void tickScheduler();

extern int schedulerType;
extern int rrQuantum;
extern int clockCycle;
extern int front,rear;
extern int readyQueue[];

// Add these external declarations
extern int mlfqFront[4];
extern int mlfqRear[4];
extern int mlfq[4][10];  // Using the same size as defined in scheduler.c

extern pthread_t schedulerThread;
void* scheduler_thread_func(void* arg);

#endif
