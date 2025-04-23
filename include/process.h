#ifndef PROCESS_H
#define PROCESS_H

typedef struct {
    int pid;
    char state[10];       // e.g., "Ready", "Running", "Blocked"
    int priority;
    int programCounter;
    int memLowerBound;
    int memUpperBound;
} PCB;

// Function prototypes
PCB createPCB(int pid, int priority, int memLower, int memUpper);

void printPCB(PCB pcb);
void setState(PCB *pcb, const char *state);
void setPriority(PCB *pcb, int priority);
void incrementPC(PCB *pcb);

#endif