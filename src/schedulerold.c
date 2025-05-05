// #include <stdio.h>
// #include "../include/scheduler.h"
// #include "../include/interpreter.h"
// #include <string.h>

// #define MAX_READY_QUEUE 10
// int readyQueue[MAX_READY_QUEUE];
// int readyCount = 0;
// int clockCycle = 0;

// void addToReadyQueue(int pid) {
//     readyQueue[readyCount++] = pid;
// }

// int popFromReadyQueue() {
//     if (readyCount == 0) return -1;
//     int pid = readyQueue[0];
//     for (int i = 1; i < readyCount; i++) {
//         readyQueue[i - 1] = readyQueue[i];
//     }
//     readyCount--;
//     return pid;
// }

// // void checkArrivals() {
// //     sortProcessesByArrival(); // Ensure order every clock cycle

// //     for (int i = 0; i < processCount; i++) {
// //         ProcessInfo *info = &processList[i];
// //         if (info->arrivalTime == clockCycle && strcmp(info->pcb.state, "Ready") == 0) {
// //             addToReadyQueue(info->pcb.pid);
// //         }
// //     }
// // }

// void checkArrivals() {
//     sortProcessesByArrival();
//     for (int i = 0; i < processCount; i++) {
//         ProcessInfo *info = &processList[i];
//         if (info->arrivalTime <= clockCycle && strcmp(info->pcb.state, "Finished") != 0) {
//             // already arrived, but maybe was not added yet
//             int alreadyInQueue = 0;
//             for (int j = 0; j < readyCount; j++) {
//                 if (readyQueue[j] == info->pcb.pid) {
//                     alreadyInQueue = 1;
//                     break;
//                 }
//             }
//             if (!alreadyInQueue && strcmp(info->pcb.state, "Blocked") != 0) {
//                 setState(&info->pcb, "Ready");

//                 addToReadyQueue(info->pcb.pid);
//             }
//         }
//     }
// }


// PCB* findPCB(int pid) {
//     for (int i = 0; i < processCount; i++) {
//         if (processList[i].pcb.pid == pid){
//             return &processList[i].pcb;
//         }
//     }
//     return NULL;
// }

// int allProcessesFinished() {
//     for (int i = 0; i < processCount; i++) {
//         if (strcmp(processList[i].pcb.state, "Ready") == 0 || strcmp(processList[i].pcb.state, "Running") == 0 || strcmp(processList[i].pcb.state, "Blocked") == 0) {
//             return 0; // still processes alive
//         }
//     }
//     return 1; // all processes finished
// }

// //FCFS
// // void runFCFS() {
// //     printf("\nStarting FCFS...\n");

// //     while (readyCount > 0 || clockCycle < 100) {  // max cycles if needed
// //         checkArrivals();

// //         if (readyCount > 0) {
// //             int pid = popFromReadyQueue();
// //             PCB *pcb = findPCB(pid);

// //             if (pcb != NULL && strcmp(pcb->state, "Ready") == 0) {
// //                 setState(pcb, "Running");

// //                 while (strcmp(pcb->state, "Running") == 0) {
// //                     executeInstruction(pcb, clockCycle);
// //                     if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
// //                         break;
// //                     }
// //                 }
// //             }
// //         }
// //         clockCycle++;
// //     }
// // }
// void runFCFS() {
//     printf("\nStarting FCFS\n");

//     while (1) { //     while (clockCycle < 30)
//         checkArrivals();
//         int run = 0;

//         if (readyCount > 0) { 
//             int pid = popFromReadyQueue();
//             PCB *pcb = findPCB(pid);

//             if (pcb != NULL && strcmp(pcb->state, "Ready") == 0) {
//                 setState(pcb, "Running");
//                 while (strcmp(pcb->state, "Running") == 0) {
//                     if (executeInstruction(pcb, clockCycle)) {
//                         clockCycle++;
//                         run = 1;
//                     } 
//                     if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
//                         break;
//                     }
//                 }
//             }
//         }
//         checkArrivals(); 

//         if(!run && readyCount == 0 && !allProcessesFinished()){
//             clockCycle++;
//         }

//         if (readyCount == 0 && allProcessesFinished()) {
//             break; 
//         }
//     }
// }

// //ROUND Rubin
// // void runRR(int quantum) {
// //     printf("\nStarting Round Robin with quantum %d...\n", quantum);

// //     while (readyCount > 0 || clockCycle < 100) { // max cycles if needed
// //         checkArrivals();

// //         if (readyCount > 0) {
// //             int pid = popFromReadyQueue();
// //             PCB *pcb = findPCB(pid);

// //             if (pcb != NULL && strcmp(pcb->state, "Ready") == 0) {
// //                 setState(pcb, "Running");

// //                 int used = 0;
// //                 while (used < quantum && strcmp(pcb->state, "Running") == 0) {
// //                     executeInstruction(pcb, clockCycle);
// //                     used++;
// //                     if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
// //                         break;
// //                     }
// //                 }

// //                 if (strcmp(pcb->state, "Running") == 0) {
// //                     setState(pcb, "Ready");
// //                     addToReadyQueue(pcb->pid);
// //                 }
// //             }
// //         }
// //         clockCycle++;
// //     }
// // }
// void runRR(int quantum) {
//     printf("\nStarting Round Robin with quantum %d\n", quantum);

//     while (readyCount > 0 || !allProcessesFinished()) {
//         checkArrivals();
//         int run = 0;

//         if (readyCount > 0) {
//             printf("Clock Cycle: %d | ReadyCount: %d\n", clockCycle, readyCount);

//             int pid = popFromReadyQueue();
//             PCB *pcb = findPCB(pid);

//             if (pcb != NULL && strcmp(pcb->state, "Ready") == 0) {
//                 setState(pcb, "Running");

//                 int used = 0;
//                 while (used < quantum && strcmp(pcb->state, "Running") == 0) {
//                     if (executeInstruction(pcb, clockCycle)) {
//                         used++;
//                         //clockCycle++;
//                         run = 1;
//                     } 
//                     if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
//                         break;
//                     }
//                     checkArrivals();  // important in case someone unblocks now

//                 }
                
//                 if (strcmp(pcb->state, "Running") == 0) {
//                     setState(pcb, "Ready");
//                     addToReadyQueue(pcb->pid); 
//                 }
//             }
//         }
//         clockCycle++;


//         // if(!run && readyCount == 0 && !allProcessesFinished()){
//         //     clockCycle++;
//         // }
//         // checkArrivals();

//         // if (readyCount == 0 && allProcessesFinished()) {
//         //     break;
//         // }
//     }
// }

// // MLFQ
// int queues[4][MAX_READY_QUEUE];
// int queueCounts[4] = {0, 0, 0, 0};
// int quantums[4] = {1, 2, 4, 8};

// void addToMLFQ(int level, int pid) {
//     queues[level][queueCounts[level]++] = pid;
// }

// int queuesNotEmpty() {
//     return queueCounts[0] || queueCounts[1] || queueCounts[2] || queueCounts[3];
// }

// void runMLFQ() {
//     printf("\nStarting MLFQ\n");

//     clockCycle = 0;

//     while (1) {
//         // Check arrivals
//         for (int i = 0; i < processCount; i++) {
//             if (processList[i].arrivalTime == clockCycle) {
//                 addToMLFQ(0, processList[i].pcb.pid); // new arrivals to highest queue
//             }
//         }

//         int foundProcess = 0;

//         for (int level = 0; level < 4; level++) {
//             if (queueCounts[level] > 0) {
//                 int pid = queues[level][0];
//                 for (int j = 1; j < queueCounts[level]; j++) {
//                     queues[level][j-1] = queues[level][j];
//                 }
//                 queueCounts[level]--;

//                 PCB *pcb = findPCBByPid(pid);

//                 if (pcb != NULL && strcmp(pcb->state, "Ready") == 0) {
//                     setState(pcb, "Running");

//                     int used = 0;
//                     while (used < quantums[level] && strcmp(pcb->state, "Running") == 0) {
//                         executeInstruction(pcb, clockCycle);
//                         used++;

//                         if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
//                             break;
//                         }
//                     }

//                     if (strcmp(pcb->state, "Running") == 0) {
//                         setState(pcb, "Ready");

//                         if (used >= quantums[level] && level < 3) {
//                             addToMLFQ(level + 1, pid); // demote
//                         } else {
//                             addToMLFQ(level, pid); // stay same level
//                         }
//                     }

//                     foundProcess = 1;
//                     break;
//                 }
//             }
//         }

//         checkArrivals();

//         if (!foundProcess && !queuesNotEmpty() && allProcessesFinished()) {
//             break; // no processes left
//         }

//         clockCycle++;
//     }
// }

// // void runMLFQ() {
// //     printf("\nStarting MLFQ...\n");

// //     while (queuesNotEmpty() || clockCycle < 100) { 
// //         for (int i = 0; i < processCount; i++) {
// //             ProcessInfo *info = &processList[i];
// //             if (info->arrivalTime == clockCycle && strcmp(info->pcb.state, "Ready") == 0) {
// //                 addToMLFQ(0, info->pcb.pid);  // Insert new arrivals at level 0
// //             }
// //         }

// //         for (int level = 0; level < 4; level++) {
// //             if (queueCounts[level] == 0) continue;

// //             int pid = queues[level][0];
// //             for (int i = 1; i < queueCounts[level]; i++) {
// //                 queues[level][i - 1] = queues[level][i];
// //             }
// //             queueCounts[level]--;

// //             PCB* pcb = findPCBByPid(pid);

// //             if (pcb != NULL && strcmp(pcb->state, "Ready") == 0) {
// //                 setState(pcb, "Running");

// //                 int used = 0;
// //                 while (used < quantums[level] && strcmp(pcb->state, "Running") == 0) {
// //                     executeInstruction(pcb, clockCycle);
// //                     used++;
// //                     if (strcmp(pcb->state, "Blocked") == 0 || strcmp(pcb->state, "Finished") == 0) {
// //                         break;
// //                     }
// //                 }

// //                 if (strcmp(pcb->state, "Running") == 0) {
// //                     setState(pcb, "Ready");
// //                     if (used >= quantums[level]) {
// //                         if (level < 3) {
// //                             addToMLFQ(level + 1, pcb->pid);  // Demote
// //                         } else {
// //                             addToMLFQ(level, pcb->pid);      // Stay
// //                         }
// //                     } else {
// //                         addToMLFQ(level, pcb->pid);  // Blocked early, stay
// //                     }
// //                 }
// //             }
// //         }
// //         clockCycle++;
// //     }
// // }

#include <stdio.h>
#include <string.h>
#include "../include/schedulerold.h"
#include "../include/process.h"
#include <string.h>

#define MAX_PROCESSES 5

int schedulerType = 1; 
int rrQuantum = 2;

int readyQueue[MAX_PROCESSES];
int front = 0, rear = 0;

int currentFCFSPid = -1;

// int currentRRIndex = -1;
// int currentRRQuantum = 0;
int rrQueue[MAX_PROCESSES];
int rrQueueSize = 0;
int rrQuantumUsed[MAX_PROCESSES] = {0};
// int lastRRPid = -1;
int currentRRPid = -1;

int mlfqQueues[4][MAX_PROCESSES];
int mlfqFront[4] = {0}, mlfqRear[4] = {0};
int mlfqQuantum[4] = {1, 2, 4, 8};
int currentMLFQLevel = 0;
int currentMLFQQuantum = 0;
int currentMLFQIndex = -1;
int quantumUsed[MAX_PROCESSES] = {0};  // Tracks quantum used per process

void initScheduler() {
    front = rear = 0;
    rrQueueSize = 0;
    // currentRRIndex = -1;
    // currentRRQuantum = 0;
    currentFCFSPid = -1;
    currentRRPid = -1;
    for (int i = 0; i < 4; i++) {
        mlfqFront[i] = 0;
        mlfqRear[i] = 0;
    }
}

void resetScheduler() {
    initScheduler();
}

// void addToReadyQueue(int pid) {
//     if (rear < MAX_PROCESSES) {
//         readyQueue[rear++] = pid;
//     }
// }
void addToReadyQueue(int pid) {
    if (schedulerType == 2) {
        if (rrQueueSize < MAX_PROCESSES) {
            rrQueue[rrQueueSize++] = pid;
        }
    } else {
        if (rear < MAX_PROCESSES) {
            readyQueue[rear++] = pid;
        }
    }
}
// void addToReadyQueue(int pid) {
//     if (schedulerType == 2) {
//         if (rrQueueSize < MAX_PROCESSES) {
//             rrQueue[rrQueueSize++] = pid;
//         }
//     } else {
//         ProcessInfo* newProc = findProcessByPid(pid);
//         if (!newProc || rear >= MAX_PROCESSES) return;

//         int insertIndex = rear;
//         for (int i = front; i < rear; i++) {
//             ProcessInfo* currProc = findProcessByPid(readyQueue[i]);
//             if (currProc && newProc->arrivalTime <= currProc->arrivalTime) {
//                 insertIndex = i;
//                 break;
//             }
//         }

//         for (int j = rear; j > insertIndex; j--) {
//             readyQueue[j] = readyQueue[j - 1];
//         }

//         readyQueue[insertIndex] = pid;
//         rear++;
//     }
// }

int getNextProcessFCFS() {
    if (currentFCFSPid != -1) {
        PCB* pcb = findPCBByPid(currentFCFSPid);
        if (pcb && strcmp(pcb->state, "Finished") != 0) {
            return currentFCFSPid; // still running or blocked
        } else {
            currentFCFSPid = -1; // allow fetching next
        }
    }

    while (front < rear) {
        int pid = readyQueue[front++];
        PCB* pcb = findPCBByPid(pid);
        if (pcb && processList[pid - 1].loaded && strcmp(pcb->state, "Ready") == 0) {
            currentFCFSPid = pid;
            return pid;
        }
    }

    // for (int i = front; i < rear; i++) {
    //     int pid = readyQueue[i];
    //     PCB* pcb = findPCBByPid(pid);
    //     if (pcb && processList[pid - 1].loaded && strcmp(pcb->state, "Ready") == 0) {
    //         // shift the rest of the queue
    //         for (int j = i; j < rear - 1; j++){
    //             readyQueue[j] = readyQueue[j + 1];
    //         }
    //         rear--;
    //         currentFCFSPid = pid;
    //         return pid;
    //     }
    // }

    return -1;
}

void removeFromRRQueue(int pid) {
    for (int i = 0; i < rrQueueSize; i++) {
        if (rrQueue[i] == pid) {
            for (int j = i; j < rrQueueSize - 1; j++) {
                rrQueue[j] = rrQueue[j + 1];
            }
            rrQueueSize--;
            break;
        }
    }
}

// int getNextProcessRR(int quantum) {
//     if (rrQueueSize == 0) return -1;

//     for (int i = 0; i < rrQueueSize; i++) {
//         int pid = rrQueue[0];
//         PCB* pcb = findPCBByPid(pid);

//         // Rotate the queue
//         // for (int j = 0; j < rrQueueSize - 1; j++)
//         //     rrQueue[j] = rrQueue[j + 1];
//         // rrQueue[rrQueueSize - 1] = pid;

//         if (!pcb || !processList[pid - 1].loaded || strcmp(pcb->state, "Finished") == 0) {
//             removeFromRRQueue(pid);
//             i--;
//             continue;
//         }

//         if (strcmp(pcb->state, "Ready") == 0)
//             return pid;
//     }

//     return -1;
// }

// int getNextProcessRR(int quantum) {
//     if (rrQueueSize == 0) return -1;

//     if (currentRRPid != -1) {
//         PCB* last = findPCBByPid(currentRRPid);
//         if (last && strcmp(last->state, "Ready") == 0 && rrQuantumUsed[currentRRPid] < quantum) {
//             rrQuantumUsed[currentRRPid]++;
//             return currentRRPid;
//         } else {
//             rrQuantumUsed[currentRRPid] = 0;
//             removeFromRRQueue(currentRRPid);
//             addToReadyQueue(currentRRPid);
//             currentRRPid = -1;
//         }
//     }

//     for (int i = 0; i < rrQueueSize; i++) {
//         int pid = rrQueue[i];
//         PCB* pcb = findPCBByPid(pid);
//         if (pcb && strcmp(pcb->state, "Ready") == 0) {
//             currentRRPid = pid;
//             rrQuantumUsed[pid] = 1;
//             return pid;
//         }
//     }
//     return -1;
// }


// int getNextProcessRR(int quantum) {
//     if (currentRRIndex == -1 || currentRRQuantum >= quantum) {
//         for (int i = 0; i < rear; i++) {
//             int index = (currentRRIndex + 1 + i) % rear;
//             PCB* pcb = findPCBByPid(readyQueue[index]);
//             if (pcb && strcmp(pcb->state, "Ready") == 0) {
//                 currentRRIndex = index;
//                 currentRRQuantum = 1;
//                 return pcb->pid;
//             }
//         }
//         return -1;
//     }
//     currentRRQuantum++;
//     return readyQueue[currentRRIndex];
// }

// int getNextProcessRR(int quantum) {
//     if (rrQueueSize == 0) return -1;

//     if (lastRRPid != -1) {
//         PCB* lastPCB = findPCBByPid(lastRRPid);
//         if (lastPCB && strcmp(lastPCB->state, "Ready") == 0 && rrQuantumUsed[lastRRPid] < quantum) {
//             rrQuantumUsed[lastRRPid]++;
//             return lastRRPid;
//         } else {
//             // Reset quantum and move it to back of queue
//             rrQuantumUsed[lastRRPid] = 0;
//             removeFromRRQueue(lastRRPid);
//             addToReadyQueue(lastRRPid); // re-add to end
//             lastRRPid = -1;
//         }
//     }

//     // Pick next ready process
//     for (int i = 0; i < rrQueueSize; i++) {
//         int pid = rrQueue[i];
//         PCB* pcb = findPCBByPid(pid);
//         if (pcb && strcmp(pcb->state, "Ready") == 0) {
//             lastRRPid = pid;
//             rrQuantumUsed[pid] = 1; // first time
//             return pid;
//         }
//     }

//     return -1;
// }

// int getNextProcessRR(int quantum) {
//     if (rrQueueSize == 0) return -1;
//     for (int i = 0; i < rrQueueSize; i++) {
//         int pid = rrQueue[0];
//         PCB* pcb = findPCBByPid(pid);

//         if (!pcb || !processList[pid - 1].loaded || strcmp(pcb->state, "Finished") == 0) {
//             removeFromRRQueue(pid);
//             i--;
//             continue;
//         }

//         if (strcmp(pcb->state, "Ready") == 0){
//             // for (int j = 0; j < rrQueueSize - 1; j++)
//             //     rrQueue[j] = rrQueue[j + 1];
//             // rrQueue[rrQueueSize - 1] = pid;
//             removeFromRRQueue(pid);
//             rrQueue[rrQueueSize++] = pid;

//             return pid;
//         }
//     }

//     return -1;
// }
int getNextProcessRR(int quantum) {
    if (rrQueueSize == 0) return -1;

    // Remove invalid or finished processes
    for (int i = 0; i < rrQueueSize; i++) {
        int pid = rrQueue[0];
        PCB* pcb = findPCBByPid(pid);

        // Remove if finished or blocked
        if (!pcb || strcmp(pcb->state, "Finished") == 0 || strcmp(pcb->state, "Blocked") == 0) {
            removeFromRRQueue(pid);
            i--;
            continue;
        }

        if (currentRRPid == -1 || currentRRPid != pid) {
            currentRRPid = pid;
            rrQuantumUsed[pid] = 1;
            return pid;
        }

        if (rrQuantumUsed[pid] < quantum) {
            rrQuantumUsed[pid]++;
            return pid;
        } else {
            // Preempt and rotate to end
            rrQuantumUsed[pid] = 0;
            removeFromRRQueue(pid);
            addToReadyQueue(pid); // re-add to end
            currentRRPid = -1;
            return getNextProcessRR(quantum); // get next
        }
    }

    return -1;
}


void addToMLFQ(int pid, int level) {
    if (!processList[pid - 1].loaded) return;
    if (level < 0) level = 0;
    if (level > 3) level = 3;
    if (mlfqRear[level] < MAX_PROCESSES) {
        mlfqQueues[level][mlfqRear[level]++] = pid;
    }
}

int getNextProcessMLFQ() {
    for (int level = 0; level < 4; level++) {
        for (int i = mlfqFront[level]; i < mlfqRear[level]; i++) {
            int pid = mlfqQueues[level][i];
            PCB* pcb = findPCBByPid(pid);

            if (!pcb || strcmp(pcb->state, "Finished") == 0 || !processList[pid - 1].loaded) {
                for (int j = i; j < mlfqRear[level] - 1; j++){
                    mlfqQueues[level][j] = mlfqQueues[level][j + 1];
                }
                mlfqRear[level]--;
                i--;
                continue;
            }

            if (strcmp(pcb->state, "Ready") == 0) {
                if (quantumUsed[pid] < mlfqQuantum[level]) {
                    return pid;
                } else {
                    quantumUsed[pid] = 0;

                    for (int j = i; j < mlfqRear[level] - 1; j++){
                        mlfqQueues[level][j] = mlfqQueues[level][j + 1];
                    }
                    mlfqRear[level]--;

                    if (level < 3){
                        addToMLFQ(pid, level + 1); 
                    }
                    else{
                        addToMLFQ(pid, level);      
                    }
                    i--;  
                }
            }
        }
    }
    return -1; 
}

int isInReadyQueue(int pid) {
    for (int i = front; i < rear; i++) {
        if (readyQueue[i] == pid) return 1;
    }
    return 0;
}

int isInMLFQ(int pid) {
    for (int l = 0; l < 4; l++) {
        for (int i = mlfqFront[l]; i < mlfqRear[l]; i++) {
            if (mlfqQueues[l][i] == pid) return 1;
        }
    }
    return 0;
}

void promoteUnblockedProcesses() {
    for (int i = 0; i < globalPcbCount; i++) {
        PCB* pcb = &pcbsGlobal[i];
        if (strcmp(pcb->state, "Ready") == 0) {
            if (schedulerType == 1) {
                if (pcb->pid != currentFCFSPid && !isInReadyQueue(pcb->pid)) {
                    addToReadyQueue(pcb->pid); 
                }
            }
            else if (schedulerType == 2){
                if (!isInReadyQueue(pcb->pid)) {
                    addToReadyQueue(pcb->pid);
                }
            }
            else if (schedulerType == 3) {
                if (!isInMLFQ(pcb->pid)) {
                    addToMLFQ(pcb->pid, pcb->priority - 1);
                }
            }
            
        }
    }
}
