#include <stdio.h>
#include "../include/logger.h"
#include "../include/process.h"
#include "../include/memory.h"
#include "../include/scheduler.h"

void log_message(const char *message) {
    // Default implementation for CLI
    printf("%s\n", message);
}

void handle_user_input(PCB *pcb, const char *var) {
    // Default implementation for CLI
    char input[256];
    printf("[Process %d] Enter value for %s: ", pcb->pid, var);
    scanf("%s", input);
    
    // Store input in memory
    char key[30];
    sprintf(key, "P%d_%s", pcb->pid, var);
    setMemory(key, input);
}