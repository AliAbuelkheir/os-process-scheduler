#include <string.h>
#include <stdio.h> 
#include "../include/mutex.h"
#include "../include/process.h"
#include "../include/memory.h"
#include "../include/scheduler.h"   

#define MAX_QUEUE 5

int userInput = -1, userOutput = -1, file = -1;
int userInputQueue[MAX_QUEUE];
int userOutputQueue[MAX_QUEUE];
int fileQueue[MAX_QUEUE];
int inputCount = 0, outputCount = 0, fileCount = 0;

void initMutexes() {
    userInput = -1;
    userOutput = -1;
    file = -1;
    inputCount = outputCount = fileCount = 0;
}

void enqueuemutex(int queue[], int* count, int pid) {
    if (*count < MAX_QUEUE) {
        queue[(*count)++] = pid;
    }
    else { 
        printf("Queue full\n"); 
    }
}

int dequeuemutex(int queue[], int* count) {
    if (*count == 0) return -1;
    int pid = queue[0];
    for (int i = 1; i < *count; i++) {
        queue[i-1] = queue[i];
    }
    (*count)--;
    return pid;
}

int semWait(PCB* pcb, const char* resource) {
    if (!pcb) return 0;
    if (strcmp(resource, "userInput") == 0) {
        if (userInput == -1 || userInput == pcb->pid) {
            userInput = pcb->pid;
            return 1;
        }
        PCB* holder = findPCBByPid(userInput);
        if (holder && pcb->priority < holder->priority) {
            setPriority(holder, pcb->priority);  // Inherit priority
        }
        enqueuemutex(userInputQueue, &inputCount, pcb->pid);
        setState(pcb, "Blocked");
        return 0;
    } 
    else if (strcmp(resource, "userOutput") == 0) {
        if (userOutput == -1 || userOutput == pcb->pid) {
            userOutput = pcb->pid;
            return 1;
        }
        PCB* holder = findPCBByPid(userOutput);
        if (holder && pcb->priority < holder->priority) {
            setPriority(holder, pcb->priority);  // Inherit priority
        }   
        enqueuemutex(userOutputQueue, &outputCount, pcb->pid);
        setState(pcb, "Blocked");
        return 0;
    }
    else if (strcmp(resource, "file") == 0) {
        if (file == -1 || file == pcb->pid) {
            file = pcb->pid;
            return 1;
        }
        PCB* holder = findPCBByPid(file);
        if (holder && pcb->priority < holder->priority) {
            setPriority(holder, pcb->priority);  // Inherit priority
        }   
        enqueuemutex(fileQueue, &fileCount, pcb->pid);
        setState(pcb, "Blocked");
        return 0;
    }
    return 1;
}

// void unblockAndReady(int* resource, int queue[], int* count, const char* resourceName) {
//     if (*count > 0) {
//         int highestPriorityIndex = 0;
//         PCB* highestPriorityPCB = findPCBByPid(queue[0]);

//         for (int i = 1; i < *count; i++) {
//             PCB* pcb = findPCBByPid(queue[i]);
//             if (pcb != NULL && pcb->priority < highestPriorityPCB->priority) {
//                 highestPriorityPCB = pcb;
//                 highestPriorityIndex = i;
//             }
//         }

//         int nextPid = queue[highestPriorityIndex];

//         for (int i = highestPriorityIndex; i < *count - 1; i++) {
//             queue[i] = queue[i + 1];
//         }
//         (*count)--;

//         PCB* pcb = findPCBByPid(nextPid);
//         if (pcb != NULL) {
//             int originalPriority = pcb->priority;
//             setState(pcb, "Ready");
//             setPriority(pcb, originalPriority);
//             if (strcmp(resourceName, "userInput") == 0) {
//                 userInput = nextPid;
//             } else if (strcmp(resourceName, "userOutput") == 0) {
//                 userOutput = nextPid;
//             } else if (strcmp(resourceName, "file") == 0) {
//                 file = nextPid;
//             }

//             if (schedulerType == 1 || schedulerType == 2)
//                 addToReadyQueue(pcb->pid);
//             else
//                 addToMLFQ(pcb->pid, pcb->priority - 1);

//             printf("Process %d unblocked, acquired %s, and moved to Ready queue\n", nextPid, resourceName);
//         }

//         *resource = nextPid;
//     } else {
//         *resource = -1;
//     }
// }

void unblockAndReady(int* resource, int queue[], int* count, const char* resourceName) {
    if (*count > 0) {
        int highestPriorityIndex = 0;
        PCB* highest = findPCBByPid(queue[0]);
        for (int i = 1; i < *count; i++) {
            PCB* candidate = findPCBByPid(queue[i]);
            if (candidate && candidate->priority < highest->priority) {
                highest = candidate;
                highestPriorityIndex = i;
            }
        }

        int nextPid = queue[highestPriorityIndex];
        for (int i = highestPriorityIndex; i < (*count) - 1; i++)
            queue[i] = queue[i + 1];
        (*count)--;

        *resource = nextPid;
        PCB* pcb = findPCBByPid(nextPid);
        if (pcb) {
            setState(pcb, "Ready");

            int queueLevel = pcb->priority - 1;
            if (queueLevel < 0) queueLevel = 0;
            if (queueLevel > 3) queueLevel = 3;

            if (schedulerType == 1 || schedulerType == 2)
                addToReadyQueue(pcb->pid);
            else
                addToMLFQ(pcb->pid, queueLevel);

            printf("Process %d unblocked, acquired %s, and moved to Q%d\n",
                   nextPid, resourceName, queueLevel);
        }
    } else {
        *resource = -1;
    }
}


void semSignal(const char* resource) {
    if (strcmp(resource, "userInput") == 0) {
        unblockAndReady(&userInput, userInputQueue, &inputCount, "userInput");
    }
    else if (strcmp(resource, "userOutput") == 0) {
        unblockAndReady(&userOutput, userOutputQueue, &outputCount, "userOutput");
    }
    else if (strcmp(resource, "file") == 0) {
        unblockAndReady(&file, fileQueue, &fileCount, "file");
    }
}
