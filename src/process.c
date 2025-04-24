#include <stdio.h>
#include <string.h>
#include "../include/process.h"

PCB createPCB(int pid, int priority, int memLower, int memUpper) {
    PCB pcb;
    pcb.pid = pid;
    strcpy(pcb.state, "Ready");
    pcb.priority = priority;
    pcb.programCounter = 0;
    pcb.memLowerBound = memLower;
    pcb.memUpperBound = memUpper;
    return pcb;
}

void printPCB(PCB pcb) {
    printf("PCB ID: %d\n", pcb.pid);
    printf("State: %s\n", pcb.state);
    printf("Priority: %d\n", pcb.priority);
    printf("Program Counter: %d\n", pcb.programCounter);
    printf("Memory Bounds: %d - %d\n", pcb.memLowerBound, pcb.memUpperBound);
}

void setState(PCB *pcb, const char *state) {
    strcpy(pcb->state, state);
}

void setPriority(PCB *pcb, int priority) {
    pcb->priority = priority;
}

void incrementPC(PCB *pcb) {
    pcb->programCounter++;
}
