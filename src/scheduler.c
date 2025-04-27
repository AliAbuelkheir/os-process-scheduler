#include "../include/scheduler.h"
#include "../include/interpreter.h"
#include <string.h>

#define MAX_READY_QUEUE 10
int readyQueue[MAX_READY_QUEUE];
int readyCount = 0;

void addToReadyQueue(int pid) {
    readyQueue[readyCount++] = pid;
}

int popFromReadyQueue() {
    if (readyCount == 0) return -1;
    int pid = readyQueue[0];
    for (int i = 1; i < readyCount; i++) {
        readyQueue[i - 1] = readyQueue[i];
    }
    readyCount--;
    return pid;
}

// void refreshReadyQueue(PCB* pcbs, int pcbCount) {
//     for (int i = 0; i < pcbCount; i++) {
//         int alreadyQueued = 0;
//         for (int j = 0; j < readyCount; j++) {
//             if (readyQueue[j] == pcbs[i].pid) {
//                 alreadyQueued = 1;
//                 break;
//             }
//         }
//         if (!alreadyQueued && strcmp(pcbs[i].state, "Ready") == 0) {
//             addToReadyQueue(pcbs[i].pid);
//         }
//     }
// }
void refreshReadyQueue(PCB* pcbs, int pcbCount) {
    readyCount = 0; // Clear the old ready queue first
    for (int i = 0; i < pcbCount; i++) {
        if (strcmp(pcbs[i].state, "Ready") == 0) {
            addToReadyQueue(pcbs[i].pid);
        }
    }
}


PCB* findPCB(PCB* pcbs, int pcbCount, int pid) {
    for (int i = 0; i < pcbCount; i++) {
        if (pcbs[i].pid == pid) return &pcbs[i];
    }
    return NULL;
}

//FCFS
void runFCFS(PCB* pcbs, int pcbCount) {
    for (int i = 0; i < pcbCount; i++) {
        addToReadyQueue(pcbs[i].pid);
    }

    while (readyCount > 0) {
        int pid = popFromReadyQueue();
        PCB* pcb = findPCB(pcbs, pcbCount, pid);

        while (strcmp(pcb->state, "Ready") == 0) {
            executeInstruction(pcb);
            if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
                break;
            }
        }
        refreshReadyQueue(pcbs, pcbCount);
    }
}

//ROUND ROBIN
void runRR(PCB* pcbs, int pcbCount, int quantum) {
    for (int i = 0; i < pcbCount; i++) {
        if (strcmp(pcbs[i].state, "Ready") == 0) {
            addToReadyQueue(pcbs[i].pid);
        }
    }

    while (readyCount > 0) {
        int pid = popFromReadyQueue();
        PCB* pcb = findPCB(pcbs, pcbCount, pid);
        int used = 0;

        while (used < quantum && strcmp(pcb->state, "Ready") == 0) {
            executeInstruction(pcb);
            used++;

            if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
                break; 
            }
        }
        refreshReadyQueue(pcbs, pcbCount);
        
        if (strcmp(pcb->state, "Ready") == 0) {
            addToReadyQueue(pcb->pid);
        }

        // for (int i = 0; i < pcbCount; i++) {
        //     if (strcmp(pcbs[i].state, "Ready") == 0) {
        //         int alreadyInQueue = 0;
        //         for (int j = 0; j < readyCount; j++) {
        //             if (readyQueue[j] == pcbs[i].pid) {
        //                 alreadyInQueue = 1;
        //                 break;
        //             }
        //         }
        //         if (!alreadyInQueue) {
        //             addToReadyQueue(pcbs[i].pid);
        //         }
        //     }
        // }
    }
}

// MLFQ
int queues[4][MAX_READY_QUEUE];
int queueCounts[4] = {0, 0, 0, 0};
int quantums[4] = {1, 2, 4, 8};

void addToMLFQ(int level, int pid) {
    queues[level][queueCounts[level]++] = pid;
}

int queuesNotEmpty() {
    return queueCounts[0] || queueCounts[1] || queueCounts[2] || queueCounts[3];
}

void runMLFQ(PCB* pcbs, int pcbCount) {
    for (int i = 0; i < pcbCount; i++) {
        addToMLFQ(0, pcbs[i].pid); // start all at level 0
    }

    while (queuesNotEmpty()) {
        for (int level = 0; level < 4; level++) {
            if (queueCounts[level] == 0) continue;

            int pid = queues[level][0];
            for (int i = 1; i < queueCounts[level]; i++) {
                queues[level][i - 1] = queues[level][i];
            }
            queueCounts[level]--;

            PCB* pcb = findPCB(pcbs, pcbCount, pid);
            int used = 0;

            while (used < quantums[level] && strcmp(pcb->state, "Ready") == 0) {
                executeInstruction(pcb);
                used++;
                if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
                    break;
                }
            }

            refreshReadyQueue(pcbs, pcbCount);

            if (strcmp(pcb->state, "Ready") == 0) {
                if (used >= quantums[level]) { // Used full quantum? Demote
                    if (level < 3) {
                        addToMLFQ(level + 1, pid);
                    } else {
                        addToMLFQ(level, pid);
                    }
                } else { // Blocked early? stay same level
                    addToMLFQ(level, pid);
                }
            }
        }
    }
}
