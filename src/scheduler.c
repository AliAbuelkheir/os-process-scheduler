#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/scheduler.h"
#include "../include/process.h"
#include "../include/interpreter.h"
#include "../include/memory.h"
#include "../include/mutex.h"
#include "../include/logger.h" 

int schedulerType = 0;
int rrQuantum = 2;
int finishedFCFS = 0;
int finishedRR = 0;
int finishedMLFQ = 0;

#define MAX_QUEUE_SIZE 10

int readyQueue[MAX_QUEUE_SIZE];
int front = 0, rear = 0;

int mlfq[4][MAX_QUEUE_SIZE];
int mlfqFront[4] = {0}, mlfqRear[4] = {0};
int mlfqQuantum[4] = {1, 2, 4, 8};

int clockCycle = 0;

void initScheduler(int type, int quantum) {
    schedulerType = type;
    rrQuantum = quantum;
}

void enqueue(int queue[], int *rear, int pid) {
    queue[(*rear)++] = pid;
}

int dequeue(int queue[], int *front, int *rear) {
    if (*front == *rear) return -1;
    int pid = queue[*front];
    for (int i = *front + 1; i < *rear; i++)
        queue[i - 1] = queue[i];
    (*rear)--;
    return pid;
}

void addToReadyQueue(int pid) {
    enqueue(readyQueue, &rear, pid);
}

void addToMLFQ(int pid, int level) {
    if (level >= 0 && level < 4)
        enqueue(mlfq[level], &mlfqRear[level], pid);
}

void loadArrivedProcesses() {
    for (int i = 0; i < processCount; i++) {
        if (processList[i].arrivalTime <= clockCycle && !processList[i].loaded) {
            processList[i].loaded = 1;
            PCB *pcb = &processList[i].pcb;
            if (schedulerType == 3){
                setState(pcb, "Ready");
                addToMLFQ(pcb->pid, 0);
            }
            else{
                setState(pcb, "Ready");
                addToReadyQueue(pcb->pid);
            }
            printf("Process %d loaded into memory at clock cycle %d.\n", pcb->pid, clockCycle);
        }
    }
}

void runFCFS() {
    int currentPid = -1;

    while (1) {
        loadArrivedProcesses();

        if (currentPid == -1 && front != rear) {
            currentPid = dequeue(readyQueue, &front, &rear);
        }

        if (currentPid != -1) {
            PCB* pcb = findPCBByPid(currentPid);

            if (!pcb || strcmp(pcb->state, "Finished") == 0) {
                currentPid = -1;
                clockCycle++;
                continue;
            }

            setState(pcb, "Running");
            int finished = executeInstruction(pcb, clockCycle);
            if (finished) { finishedFCFS++; }

            if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
                currentPid = -1;
            }
        }
        clockCycle++;
        
        if (finishedFCFS == processCount){
            printf("Program finished execution.\n");
            break;
        }
    }
}

void runRR() {
    int quantumCounter = 0;

    while (1) {
        if (front == rear) {
            loadArrivedProcesses();
            clockCycle++;
        }

        int pid = dequeue(readyQueue, &front, &rear);
        PCB *pcb = findPCBByPid(pid);
        if (!pcb || strcmp(pcb->state, "Finished") == 0) continue;

        setState(pcb, "Running");
        quantumCounter = 0;

        while (quantumCounter < rrQuantum) {
            loadArrivedProcesses();
            int finished = executeInstruction(pcb, clockCycle);
            if (finished) { finishedRR++; }            
            clockCycle++;
            quantumCounter++;

            if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0)
                break;
        }

        if (strcmp(pcb->state, "Ready") == 0 || strcmp(pcb->state, "Running") == 0) {
            setState(pcb, "Ready");
            addToReadyQueue(pcb->pid);  
        }
        
        if (finishedRR == processCount){
            printf("Program finished execution.\n");
            break;
        }
    }
}

void runMLFQ() {
    while (1) {
        int level = -1, pid = -1;

        for (int i = 0; i < 4; i++) {
            if (mlfqFront[i] != mlfqRear[i]) {
                pid = dequeue(mlfq[i], &mlfqFront[i], &mlfqRear[i]);
                level = i;
                break;
            }
        }

        if (pid == -1) {
            loadArrivedProcesses();
            clockCycle++;
        }

        PCB *pcb = findPCBByPid(pid);
        if (!pcb || strcmp(pcb->state, "Finished") == 0) {
            continue;
        }
        
        setState(pcb, "Running");
        int quantum = mlfqQuantum[level];
        int count = 0;

        while (count < quantum) {
            loadArrivedProcesses();  
            int finished = executeInstruction(pcb, clockCycle);
            if (finished) { finishedMLFQ++; }
            clockCycle++;
            count++;

            if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0)
                break;
        }

        if (strcmp(pcb->state, "Ready") == 0 || strcmp(pcb->state, "Running") == 0) {
            int nextLevel = (level == 3) ? 3 : level + 1;
            setPriority(pcb, nextLevel + 1);
            addToMLFQ(pcb->pid, nextLevel);
            setState(pcb, "Ready");
        }
        
        if (finishedMLFQ == processCount){
            printf("Program finished execution.\n");
            break;
        }
    }
}

int getFinishedProcessCount() {
    switch (schedulerType) {
        case 1: // FCFS
            return finishedFCFS;
        case 2: // Round Robin
            return finishedRR;
        case 3: // MLFQ
            return finishedMLFQ;
        default:
            return 0;
    }
}

int getTotalProcessCount() {
    return processCount;
}

int getCurrentClockCycle() {
    return clockCycle;
}

int getCurrentRunningProcess() {
    for (int i = 0; i < processCount; i++) {
        if (strcmp(processList[i].pcb.state, "Running") == 0) {
            return processList[i].pcb.pid;
        }
    }
    return -1; 
}

void startScheduler() {
    printf("Starting scheduler...\n");
    switch (schedulerType) {
        case 1:
            runFCFS();
            break;
        case 2:
            runRR();
            break;
        case 3:
            runMLFQ();
            break;
        default:
            printf("Unknown scheduler type.\n");
    }
}

void advanceScheduler() {
    loadArrivedProcesses();

    switch (schedulerType) {
        case 1: // FCFS
            runFCFS();
            break;
        case 2: // Round Robin
            runRR();
            break;
        case 3: // MLFQ
            runMLFQ();
            break;
        default:
            printf("Unknown scheduler type.\n");
    }
}

void resetScheduler() {
    initMemory();
    initMutexes();
    front = rear = 0;
    for (int i = 0; i < 4; i++) {
        mlfqFront[i] = mlfqRear[i] = 0;
    }
    processCount = 0;
    globalPcbCount = 0;
    clockCycle = 0;
    finishedFCFS = finishedRR = finishedMLFQ = 0;
}
