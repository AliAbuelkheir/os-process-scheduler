//process.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/process.h"
#include "../include/fileio.h"
#include "../include/memory.h"

PCB pcbsGlobal[3];  
int globalPcbCount = 0;

PCB createProcess(int pid, int priority, const char* filename) {
    PCB pcb;
    pcb.pid = pid;
    strcpy(pcb.state, "Ready");
    setPriority(&pcb, priority);
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

    // 7. Store it globally
    pcbsGlobal[globalPcbCount++] = pcb;

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
    if (!pcb) return;
    strcpy(pcb->state, state);
    char key[30];
    sprintf(key, "P%d_state", pcb->pid);
    setMemory(key, state);
}

const char* getState(PCB *pcb) {
    if (!pcb) {
        return NULL;
    }
    char key[30];
    sprintf(key, "P%d_state", pcb->pid);
    const char* state = getMemory(key);
    return state ? state : pcb->state;
    //return pcb->state;
}

void setPriority(PCB *pcb, int priority) {
    pcb->priority = priority;
}

int getPriority(int pid){
    for (int i = 0; i < processCount; i++) {
        if (processList[i].pid == pid) {
            return processList[i].pcb.priority;
        }
    }
    return -1; // Return -1 if the process is not found
}

void incrementPC(PCB *pcb) {
    pcb->programCounter++;
    char key[30], value[10];
    sprintf(key, "P%d_pc", pcb->pid);
    sprintf(value, "%d", pcb->programCounter);
    setMemory(key, value);
}

PCB* findPCBByPid(int pid) {
    for (int i = 0; i < globalPcbCount; i++) {
        if (pcbsGlobal[i].pid == pid) {
            return &pcbsGlobal[i];
        }
    }
    return NULL;
}
ProcessInfo* findProcessByPid(int pid) {
    for (int i = 0; i < processCount; i++) {
        if (processList[i].pcb.pid == pid) {
            return &processList[i];
        }
    }
    return NULL;
}


ProcessInfo processList[10];
int processCount = 0;

void addProcess(const char* filename, int arrivalTime) {
    PCB pcb = createProcess(processCount + 1, processCount + 1, filename);
    
    processList[processCount].pid = pcb.pid;
    processList[processCount].arrivalTime = arrivalTime;
    processList[processCount].loaded = 0;
    processList[processCount].pcb = pcb;
    strcpy(processList[processCount].sourceFile, filename); 

    processCount++;
}

void sortProcessesByArrival() {
    for (int i = 0; i < processCount - 1; i++) {
        for (int j = i + 1; j < processCount; j++) {
            if (processList[i].arrivalTime > processList[j].arrivalTime) {
                ProcessInfo temp = processList[i];
                processList[i] = processList[j];
                processList[j] = temp;
            }
        }
    }
}
