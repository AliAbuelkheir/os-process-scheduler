#ifndef SCHEDULER_H
#define SCHEDULER_H

void initScheduler(int type, int quantum);
void addToReadyQueue(int pid);
void addToMLFQ(int pid, int level);
void startScheduler();

extern int schedulerType;
extern int rrQuantum;
extern int clockCycle;
extern int front,rear;
extern int readyQueue[];


// Add these external declarations
extern int mlfqFront[4];
extern int mlfqRear[4];
extern int mlfq[4][10];  // Using the same size as defined in scheduler.c


#endif
