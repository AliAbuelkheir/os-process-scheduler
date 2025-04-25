#ifndef PROCESS_H
#define PROCESS_H

typedef struct {
    int pid;
    char state[10];       // Ready, Running, Blocked
    int priority;
    int programCounter;
    int memLowerBound;
    int memUpperBound;
} PCB;

PCB createProcess(int pid, int priority, const char* filename);

void printPCB(PCB pcb);
void setState(PCB *pcb, const char *state);
void setPriority(PCB *pcb, int priority);
void incrementPC(PCB *pcb);

#endif