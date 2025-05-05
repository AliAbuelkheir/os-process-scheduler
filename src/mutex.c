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
    if (strcmp(resource, "userInput") == 0) {
        if (userInput == -1 || userInput == pcb->pid) {
            userInput = pcb->pid;
            return 1;
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
        enqueuemutex(userOutputQueue, &outputCount, pcb->pid);
        setState(pcb, "Blocked");
        return 0;
    }
    else if (strcmp(resource, "file") == 0) {
        if (file == -1 || file == pcb->pid) {
            file = pcb->pid;
            return 1;
        }
        enqueuemutex(fileQueue, &fileCount, pcb->pid);
        setState(pcb, "Blocked");
        return 0;
    }
    return 1;
}

// int unblockAndReady(int* resource, int queue[], int* count) {
//     if (*count > 0) {
//         int highestPriorityIndex = 0;
//         PCB* highestPriorityPCB = findPCBByPid(queue[0]);
        
//         for (int i = 1; i < *count; i++) {
//             PCB* pcb = findPCBByPid(queue[i]);
//             if (pcb != NULL && pcb->priority < highestPriorityPCB->priority) { // Smaller number = higher priority (priority 1 is higher than 2)
//                 highestPriorityPCB = pcb;
//                 highestPriorityIndex = i;
//             }
//         }
        
//         int nextPid = queue[highestPriorityIndex];
        
//         // Remove nextPid from queue
//         for (int i = highestPriorityIndex; i < *count - 1; i++) {
//             queue[i] = queue[i + 1];
//         }
//         (*count)--;
        
//         PCB* pcb = findPCBByPid(nextPid);
//         if (pcb != NULL) {
//             setState(pcb, "Ready");
//             addToReadyQueue(pcb->pid);
//             printf("Process %d unblocked and moved to Ready queue\n", nextPid);
//             return 1;
//         }
//         *resource = nextPid;
//     } else {
//         *resource = -1;
//     }
//     return 0;
// }

void unblockAndReady(int* resource, int queue[], int* count, const char* resourceName) {
    if (*count > 0) {
        int highestPriorityIndex = 0;
        PCB* highestPriorityPCB = findPCBByPid(queue[0]);

        for (int i = 1; i < *count; i++) {
            PCB* pcb = findPCBByPid(queue[i]);
            if (pcb != NULL && pcb->priority < highestPriorityPCB->priority) {
                highestPriorityPCB = pcb;
                highestPriorityIndex = i;
            }
        }

        int nextPid = queue[highestPriorityIndex];

        for (int i = highestPriorityIndex; i < *count - 1; i++) {
            queue[i] = queue[i + 1];
        }
        (*count)--;

        PCB* pcb = findPCBByPid(nextPid);
        if (pcb != NULL) {
            setState(pcb, "Ready");

            if (strcmp(resourceName, "userInput") == 0) {
                userInput = nextPid;
            } else if (strcmp(resourceName, "userOutput") == 0) {
                userOutput = nextPid;
            } else if (strcmp(resourceName, "file") == 0) {
                file = nextPid;
            }

            if (schedulerType == 1 || schedulerType == 2)
                addToReadyQueue(pcb->pid);
            else
                addToMLFQ(pcb->pid, pcb->priority - 1);

            printf("Process %d unblocked, acquired %s, and moved to Ready queue\n", nextPid, resourceName);
        }

        *resource = nextPid;
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
