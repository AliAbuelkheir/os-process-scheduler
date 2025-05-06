#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/process.h"
#include "../include/memory.h"
#include "../include/mutex.h"
#include "../include/scheduler.h"

int main() {
    initMemory();
    initMutexes();

    int schedulerChoice;
    int quantum = 0;

    printf("Select Scheduler Algorithm:\n");
    printf("1. First Come First Serve (FCFS)\n");
    printf("2. Round Robin (RR)\n");
    printf("3. Multilevel Feedback Queue (MLFQ)\n");
    printf("Enter choice: ");
    scanf("%d", &schedulerChoice);

    if (schedulerChoice == 2) {
        printf("Enter quantum for Round Robin: ");
        scanf("%d", &quantum);
    }

    initScheduler(schedulerChoice, quantum);

    int numProcesses;
    printf("Enter number of processes to load: ");
    scanf("%d", &numProcesses);

    for (int i = 0; i < numProcesses; i++) {
        char filename[100];
        int arrivalTime;

        printf("Enter filename for process %d: ", i + 1);
        scanf("%s", filename);

        printf("Enter arrival time for process %d: ", i + 1);
        scanf("%d", &arrivalTime);

        addProcess(filename, arrivalTime);
    }

    sortProcessesByArrival();
    startScheduler();
    printMemory();

    return 0;
}
