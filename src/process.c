#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/process.h"
#include "../include/fileio.h"
#include "../include/memory.h"

PCB createProcess(int pid, int priority, const char* filename) {
    PCB pcb;
    pcb.pid = pid;
    strcpy(pcb.state, "Ready");
    pcb.priority = priority;
    pcb.programCounter = 0;


    // 1. Read program file
    char* lines[MAX_LINES];
    int lineCount = readProgramFromFile(filename, lines);

    // 2. Calculate required space
    int requiredMemory = 6 + lineCount;
    int startIndex = findFreeBlock(requiredMemory);  // implement this in memory.c

    if (startIndex == -1) {
        printf("Error: No enough memory to create process %d\n", pid);
        pcb.memLowerBound = -1;
        pcb.memUpperBound = -1;
        return pcb;
    }

    // 3. Save PCB fields in memory
    char key[30], value[50];

    sprintf(key, "P%d_state", pid);
    setMemory(key, "Ready");

    sprintf(key, "P%d_pc", pid);
    setMemory(key, "0");

    sprintf(key, "P%d_priority", pid);
    sprintf(value, "%d", priority);
    setMemory(key, value);

    // 4. Save instructions
    for (int i = 0; i < lineCount; i++) {
        sprintf(key, "P%d_line_%d", pid, i);
        setMemory(key, lines[i]);
        free(lines[i]);  // cleanup
    }

    //5. Save 3 variables in memory
    sprintf(key, "P%d_var1", pid);
    setMemory(key, "");
    sprintf(key, "P%d_var2", pid);
    setMemory(key, "");
    sprintf(key, "P%d_var3", pid);
    setMemory(key, "");

    // 6. Set PCB memory bounds
    pcb.memLowerBound = startIndex;
    pcb.memUpperBound = startIndex + requiredMemory - 1;

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
