//WON'T USE THIS

// int main() {
//     initMemory();   // your memory setup
//     initMutexes();  // your mutex setup

//     int n;
//     printf("Enter number of programs to load: ");
//     scanf("%d", &n);

//     for (int i = 0; i < n; i++) {
//         char filename[100];
//         int arrival;
//         printf("Enter filename for process %d: ", i+1);
//         scanf("%s", filename);
//         printf("Enter arrival time for process %d: ", i+1);
//         scanf("%d", &arrival);
//         addProcess(filename, arrival);
//     }

//     sortProcessesByArrival();

//     printf("\nChoose a scheduling algorithm:\n");
//     printf("1. First-Come First-Serve (FCFS)\n");
//     printf("2. Round Robin (RR)\n");
//     printf("3. Multi-Level Feedback Queue (MLFQ)\n");
//     printf("Enter choice: ");
//     int choice;
//     scanf("%d", &choice);

//     if (choice == 1) {
//         runFCFS();   
//     } else if (choice == 2) {
//         printf("Enter quantum for RR: ");
//         int quantum;
//         scanf("%d", &quantum);
//         runRR(quantum);  
//     } else if (choice == 3) {
//         runMLFQ();
//     } else {
//         printf("Invalid choice.\n");
//     }

//     printf("\n=== Final Memory Dump ===\n");
//     printMemory();
//     printf("\n=== Program Completed Successfully ===\n");

//     return 0;
// }
#include "../include/fileio.h"
#include "../include/memory.h"
#include "../include/process.h"
#include "../include/interpreter.h"
#include "../include/schedulerold.h"
#include <stdio.h>
#include <string.h>
#define MAX_PROCESSES 5

int main() {
    initMemory();
    initScheduler();
    int rrQuantumUsed[MAX_PROCESSES] = {0};

    printf("Choose a scheduling algorithm:\n");
    printf("1. First-Come First-Serve (FCFS)\n");
    printf("2. Round Robin (RR)\n");
    printf("3. Multi-Level Feedback Queue (MLFQ)\n");
    printf("Enter choice: ");
    scanf("%d", &schedulerType);

    if (schedulerType == 2) {
        printf("Enter quantum for RR: ");
        scanf("%d", &rrQuantum);
    }

    int num;
    printf("Enter number of programs to load: ");
    scanf("%d", &num);

    for (int i = 0; i < num; i++) {
        char filename[100];
        int arrival;
        printf("Enter filename for process %d: ", i + 1);
        scanf("%s", filename);
        printf("Enter arrival time for process %d: ", i + 1);
        scanf("%d", &arrival);
        addProcess(filename, arrival);
    }

    sortProcessesByArrival();

    int minArrival = processList[0].arrivalTime;
    for (int i = 1; i < processCount; i++) {
        if (processList[i].arrivalTime < minArrival)
            minArrival = processList[i].arrivalTime;
    }

    int clockCycle = minArrival;
    // int clockCycle = processList[0].arrivalTime;

    int finishedCount = 0;
    int markedFinished[MAX_PROCESSES] = {0}; 
    int currentRRPid = -1;
    int currentRRQuantumUsed = 0; 

    while (finishedCount < processCount) {
        for (int i = 0; i < processCount; i++) {
            if (processList[i].arrivalTime == clockCycle && !processList[i].loaded) {
                PCB *pcb = &processList[i].pcb;
                processList[i].loaded = 1;
                if (schedulerType == 1)
                    addToReadyQueue(pcb->pid);
                else if (schedulerType == 2)
                    addToReadyQueue(pcb->pid);  
                else
                    addToMLFQ(pcb->pid, pcb->priority - 1);
                printf("Process %d loaded at clock %d\n", pcb->pid, clockCycle);
            }
        }
    
        promoteUnblockedProcesses();
    
        int pid = -1;
    
        if (schedulerType == 2) {
            if (currentRRPid == -1 || currentRRQuantumUsed >= rrQuantum || (findPCBByPid(currentRRPid) && strcmp(findPCBByPid(currentRRPid)->state, "Finished") == 0)) {
                currentRRPid = getNextProcessRR(rrQuantum);
                currentRRQuantumUsed = 0;
            }
            pid = currentRRPid;
        }
        else if (schedulerType == 1) {
            pid = getNextProcessFCFS();
        } else if (schedulerType == 3) {
            pid = getNextProcessMLFQ();
        }
    
        if (pid != -1) {
            PCB *pcb = findPCBByPid(pid);
            if (pcb && strcmp(pcb->state, "Ready") == 0) {
                setState(pcb, "Running");
                executeInstruction(pcb, clockCycle);
    
                if (strcmp(pcb->state, "Running") == 0)
                    setState(pcb, "Ready");
    
                if (strcmp(pcb->state, "Finished") == 0 && markedFinished[pcb->pid] == 0) {
                    markedFinished[pcb->pid] = 1;
                    finishedCount++;
                    if (schedulerType == 2) {
                        currentRRPid = -1; 
                    }
                }
    
                if (schedulerType == 2 && strcmp(pcb->state, "Ready") == 0) {
                    currentRRQuantumUsed++;
                    if (currentRRQuantumUsed >= rrQuantum) {
                        currentRRPid = -1;
                        currentRRQuantumUsed = 0;
                    }
                }
    
                if (schedulerType == 3 && strcmp(pcb->state, "Ready") == 0)
                    quantumUsed[pid]++;
            }
        } else {
            printf("Clock Cycle: %d — CPU idle\n", clockCycle);
        }
    
        clockCycle++;
    }
    

    printf("All processes finished at clock cycle %d.\n", clockCycle);
    printMemory();
    return 0;
}
