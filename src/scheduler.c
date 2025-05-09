#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/scheduler.h"
#include "../include/process.h"
#include "../include/interpreter.h"
#include "../include/memory.h"
#include "../include/mutex.h"
#include "../include/gui_main.h"
#include <pthread.h>

int schedulerType = 0;
int rrQuantum;
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

int rrStepInitialized = 0;
int mlfqStepInitialized = 0;

void initScheduler(int type, int quantum) {
    schedulerType = type;
    rrQuantum = quantum;
    printf("DEBUG: Current RR Quantum = %d\n", rrQuantum);  

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
            char msg[256];
            sprintf(msg, "Process %d loaded into memory at clock cycle %d.", pcb->pid, clockCycle);
            log_message(msg);

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
            char msg[256];
            sprintf(msg, "Program finished execution.");
            log_message(msg);

            break;
        }
    }
}

static int currentFCFSPid = -1;

void tickFCFS() {
    char msg[256];
    loadArrivedProcesses();

    if (currentFCFSPid == -1 && front != rear) {
        currentFCFSPid = dequeue(readyQueue, &front, &rear);
    }

    if (currentFCFSPid != -1) {
        PCB* pcb = findPCBByPid(currentFCFSPid);
        if (!pcb || strcmp(pcb->state, "Finished") == 0) {
            currentFCFSPid = -1;
        } else {
            setState(pcb, "Running");
            const char* instr = getCurrentInstruction(pcb);
            if (instr != NULL) {
                sprintf(msg, "[Cycle %d] P%d executed instruction: %s", 
                        clockCycle, pcb->pid, instr);
                log_message(msg);
            }
            int finished = executeInstruction(pcb, clockCycle);
            if (finished){
                finishedFCFS++;
                sprintf(msg, "P%d Finished execution.", pcb->pid);
                log_message(msg);
            } 
            if (strcmp(pcb->state, "Blocked") == 0){
                currentFCFSPid = -1;
                sprintf(msg, "P%d got blocked.", pcb->pid);
                log_message(msg);
            }
            if (strcmp(pcb->state, "Finished") == 0){
                currentFCFSPid = -1;
            }
        }
    }

    clockCycle++;
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
            char msg[256];
            sprintf(msg, "Program finished execution.");
            log_message(msg);
            break;
        }
    }
}
// Add these static variables at the top of scheduler.c with other globals
static PCB *currentRRProcess = NULL;
static int rrQuantumUsed = 0;

void tickRR() {
    char msg[256];
    loadArrivedProcesses();

    if (!currentRRProcess && front != rear) {
        int nextPid = dequeue(readyQueue, &front, &rear);
        currentRRProcess = findPCBByPid(nextPid);
        rrQuantumUsed = 0;

        if (currentRRProcess) {
            setState(currentRRProcess, "Running");
        }
    }

    if (currentRRProcess) {
        const char* instr = getCurrentInstruction(currentRRProcess);
        if (instr) {
            sprintf(msg, "[Cycle %d] P%d executing: %s", clockCycle, currentRRProcess->pid, instr);
            log_message(msg);
        }

        int result = executeInstruction(currentRRProcess, clockCycle);
        rrQuantumUsed++;

        const char* state = getState(currentRRProcess);

        if (result) {
            sprintf(msg, "P%d finished execution", currentRRProcess->pid);
            log_message(msg);
            finishedRR++;
            currentRRProcess = NULL;
            rrQuantumUsed = 0;
        }
        else if (strcmp(state, "Blocked") == 0) {
            sprintf(msg, "P%d got blocked", currentRRProcess->pid);
            log_message(msg);
            currentRRProcess = NULL;
            rrQuantumUsed = 0;
        }
        else if (rrQuantumUsed >= rrQuantum) {
            sprintf(msg, "P%d quantum expired → back to ready queue", currentRRProcess->pid);
            log_message(msg);
            setState(currentRRProcess, "Ready");
            addToReadyQueue(currentRRProcess->pid);
            currentRRProcess = NULL;
            rrQuantumUsed = 0;
        }
    }
    clockCycle++;
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
            char msg[256];
            sprintf(msg, "Program finished execution.");
            log_message(msg);
            break;
        }
    }
}

static PCB *currentMLFQProcess = NULL;
static int mlfqLevel = -1;
static int mlfqQuantumUsed = 0;

// void tickMLFQ() {
//     loadArrivedProcesses();

//     // Check if we need a new process
//     if (!currentMLFQProcess || 
//         strcmp(currentMLFQProcess->state, "Blocked") == 0 || 
//         strcmp(currentMLFQProcess->state, "Finished") == 0 || 
//         mlfqQuantumUsed >= mlfqQuantum[mlfqLevel]) {

//         // Handle quantum expiration for running process
//         if (currentMLFQProcess && strcmp(currentMLFQProcess->state, "Running") == 0) {
//             int nextLevel = (mlfqLevel == 3) ? 3 : mlfqLevel + 1;
//             setPriority(currentMLFQProcess, nextLevel + 1);
//             setState(currentMLFQProcess, "Ready");
//             addToMLFQ(currentMLFQProcess->pid, nextLevel);
//         }

//         currentMLFQProcess = NULL;

//         // Select next process from highest priority queue
//         for (int i = 0; i < 4; i++) {
//             while (mlfqFront[i] != mlfqRear[i]) {
//                 int pid = dequeue(mlfq[i], &mlfqFront[i], &mlfqRear[i]);
//                 PCB* nextProcess = findPCBByPid(pid);

//                 // Only select Ready processes that have arrived
//                 if (nextProcess && 
//                     strcmp(nextProcess->state, "Ready") == 0 && 
//                     processList[pid - 1].arrivalTime <= clockCycle) {
//                     currentMLFQProcess = nextProcess;
//                     mlfqLevel = i;
//                     mlfqQuantumUsed = 0;
//                     break;
//                 } else {
//                     addToMLFQ(pid, i);  // Put back if not ready
//                 }
//             }
//             if (currentMLFQProcess) break;
//         }
//     }

//     // Execute current process
//     if (currentMLFQProcess) {
//         setState(currentMLFQProcess, "Running");
//         executeInstruction(currentMLFQProcess, clockCycle);
//         mlfqQuantumUsed++;
//     }

//     clockCycle++;
// }

// void tickMLFQ() {
//     loadArrivedProcesses();

//     if (!currentMLFQProcess || 
//         strcmp(currentMLFQProcess->state, "Blocked") == 0 || 
//         strcmp(currentMLFQProcess->state, "Finished") == 0 || 
//         mlfqQuantumUsed >= mlfqQuantum[mlfqLevel]) {

//         if (currentMLFQProcess && strcmp(currentMLFQProcess->state, "Running") == 0) {
//             int nextLevel = (mlfqLevel == 3) ? 3 : mlfqLevel + 1;
//             setPriority(currentMLFQProcess, nextLevel + 1);
//             setState(currentMLFQProcess, "Ready");
//             addToMLFQ(currentMLFQProcess->pid, nextLevel);

//             char demoteMsg[256];
//             sprintf(demoteMsg, "[Cycle %d] P%d quantum expired → demoted to Q%d", 
//                     clockCycle, currentMLFQProcess->pid, nextLevel);
//             log_message(demoteMsg);
//         }

//         currentMLFQProcess = NULL;

//         for (int i = 0; i < 4; i++) {
//             if (mlfqFront[i] != mlfqRear[i]) {
//                 int pid = dequeue(mlfq[i], &mlfqFront[i], &mlfqRear[i]);
//                 currentMLFQProcess = findPCBByPid(pid);
//                 mlfqLevel = i;
//                 mlfqQuantumUsed = 0;

//                 char fetchMsg[256];
//                 sprintf(fetchMsg, "[Cycle %d] P%d fetched from Q%d", clockCycle, pid, i);
//                 log_message(fetchMsg);
//                 break;
//             }
//         }
//     }

//     if (currentMLFQProcess) {
//         setState(currentMLFQProcess, "Running");

//         const char* instr = getCurrentInstruction(currentMLFQProcess);
//         int finished = executeInstruction(currentMLFQProcess, clockCycle);
//         const char* state = getState(currentMLFQProcess);

//         if (instr) {
//             char msg[256];
//             sprintf(msg, "[Cycle %d] P%d executed instruction: %s | State: %s", 
//                     clockCycle, currentMLFQProcess->pid, instr, state);
//             log_message(msg);
//         }

//         if (finished) {
//             finishedMLFQ++;
//             char finishMsg[128];
//             sprintf(finishMsg, "[Cycle %d] P%d Finished execution.", clockCycle, currentMLFQProcess->pid);
//             log_message(finishMsg);
//             currentMLFQProcess = NULL;
//             mlfqQuantumUsed = 0;
//         } 
//         else if (strcmp(state, "Blocked") == 0) {
//             char blockMsg[128];
//             sprintf(blockMsg, "[Cycle %d] P%d got blocked.", clockCycle, currentMLFQProcess->pid);
//             log_message(blockMsg);
//             currentMLFQProcess = NULL;
//             mlfqQuantumUsed = 0;
//         } 
//         else {
//             mlfqQuantumUsed++;
//         }
//     }

//     clockCycle++;
// }

void tickMLFQ() {
    loadArrivedProcesses();

    // Demote if time slice ended or blocked/finished
    if (currentMLFQProcess &&
        (strcmp(currentMLFQProcess->state, "Blocked") == 0 ||
         strcmp(currentMLFQProcess->state, "Finished") == 0 ||
         mlfqQuantumUsed >= mlfqQuantum[mlfqLevel])) {

        if (strcmp(currentMLFQProcess->state, "Running") == 0) {
            int nextLevel = (mlfqLevel == 3) ? 3 : mlfqLevel + 1;
            setPriority(currentMLFQProcess, nextLevel + 1);
            setState(currentMLFQProcess, "Ready");
            addToMLFQ(currentMLFQProcess->pid, nextLevel);
        }

        currentMLFQProcess = NULL;
        mlfqQuantumUsed = 0;
    }

    // Preemption check: if a higher priority process is ready
    if (currentMLFQProcess) {
        for (int i = 0; i < mlfqLevel; i++) {
            for (int j = mlfqFront[i]; j < mlfqRear[i]; j++) {
                PCB* pcb = findPCBByPid(mlfq[i][j]);
                if (pcb && strcmp(pcb->state, "Ready") == 0) {
                    setState(currentMLFQProcess, "Ready");
                    addToMLFQ(currentMLFQProcess->pid, mlfqLevel);

                    char msg[128];
                    sprintf(msg, "[Cycle %d] P%d preempted by higher priority P%d", clockCycle, currentMLFQProcess->pid, pcb->pid);
                    log_message(msg);

                    currentMLFQProcess = NULL;
                    mlfqQuantumUsed = 0;
                    break;
                }
            }
            if (!currentMLFQProcess) break;
        }
    }

    // Fetch if no current process
    if (!currentMLFQProcess) {
        for (int i = 0; i < 4; i++) {
            while (mlfqFront[i] != mlfqRear[i]) {
                int pid = dequeue(mlfq[i], &mlfqFront[i], &mlfqRear[i]);
                PCB* pcb = findPCBByPid(pid);
                if (pcb && strcmp(pcb->state, "Ready") == 0) {
                    currentMLFQProcess = pcb;
                    mlfqLevel = i;
                    mlfqQuantumUsed = 0;
                    char msg[128];
                    sprintf(msg, "[Cycle %d] P%d fetched from Q%d", clockCycle, pid, i);
                    log_message(msg);
                    break;
                } else {
                    addToMLFQ(pid, i); // requeue
                }
            }
            if (currentMLFQProcess) break;
        }
    }

    // Execute
    if (currentMLFQProcess) {
        setState(currentMLFQProcess, "Running");
        const char *instr = getCurrentInstruction(currentMLFQProcess);
        int finished = executeInstruction(currentMLFQProcess, clockCycle);
        const char *state = getState(currentMLFQProcess);

        if (instr) {
            char msg[256];
            sprintf(msg, "[Cycle %d] P%d executing instruction: %s", clockCycle, currentMLFQProcess->pid, instr);
            log_message(msg);
        }

        if (finished) {
            char msg[128];
            sprintf(msg, "[Cycle %d] P%d Finished execution.", clockCycle, currentMLFQProcess->pid);
            log_message(msg);
            finishedMLFQ++;
            currentMLFQProcess = NULL;
            mlfqQuantumUsed = 0;
        } else if (strcmp(state, "Blocked") == 0) {
            char msg[128];
            sprintf(msg, "[Cycle %d] P%d got blocked.", clockCycle, currentMLFQProcess->pid);
            log_message(msg);
            currentMLFQProcess = NULL;
            mlfqQuantumUsed = 0;
        } else {
            mlfqQuantumUsed++;
        }
    }

    clockCycle++;
}


int getFinishedProcessCount() {
    switch (schedulerType) {
        case 1: 
            return finishedFCFS;
        case 2: 
            return finishedRR;
        case 3: 
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

void tickScheduler() {
    char msg[64];
    if (getFinishedProcessCount() >= getTotalProcessCount()) {
        return;  
    }
    switch (schedulerType) {
        case 1:
            tickFCFS();
            break;
        case 2:
            tickRR();
            break;
        case 3:
            tickMLFQ();
            break;
        default:
            sprintf(msg, "Invalid scheduler type in tickScheduler()");
            log_message(msg);
    }
}

void resetScheduler() {
    initMemory();
    initMutexes();
    front = rear = 0;
    rrQuantum = 0;
    for (int i = 0; i < 4; i++) {
        mlfqFront[i] = mlfqRear[i] = 0;
    }
    processCount = 0;
    globalPcbCount = 0;
    clockCycle = 0;
    finishedFCFS = finishedRR = finishedMLFQ = 0;
}
