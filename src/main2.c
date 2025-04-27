#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "../include/process.h"
#include "../include/memory.h"
#include "../include/fileio.h"
#include "../include/mutex.h"
#include "../include/scheduler.h" 

int main(){

    // Initialize memory and PCB table
    initMemory();
    initMutexes();
    const char* programFiles[] = {
        "programs/Program_1.txt",
        "programs/Program_2.txt",
        "programs/Program_3.txt",
    };
    
    PCB pcbs[3];
    for (int i = 0; i < 3; i++) {
        pcbs[i] = createProcess(i + 1, i + 1, programFiles[i]);
        printPCB(pcbs[i]);
        //printf("-------------------------\n");
    }
    int choice;
    printf("\nChoose a scheduling algorithm:\n");
    printf("1. First-Come First-Serve (FCFS)\n");
    printf("2. Round Robin (RR)\n");
    printf("3. Multilevel Feedback Queue (MLFQ)\n");
    printf("Enter choice: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            runFCFS(pcbs, 3);   
            break;
        case 2: {
            int quantum;
            printf("Enter quantum: ");
            scanf("%d", &quantum);
            runRR(pcbs, 3, quantum);
            break;
        }        
        case 3:
            runMLFQ(pcbs, 3);
            break;
        default:
            printf("Invalid choice!\n");
            return 1;
    }

    printf("\n=== Final Memory Dump ===\n");
    printMemory();

    printf("\n=== Program Completed Successfully ===\n");

    return 0;
}