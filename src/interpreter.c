#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/interpreter.h"
#include "../include/memory.h"
#include "../include/process.h"
#include "../include/fileio.h"

void executeInstruction(PCB *pcb) {
    char key[30];
    sprintf(key, "P%d_line_%d", pcb->pid, pcb->programCounter);

    const char* instruction = getMemory(key);
    if (instruction == NULL) {
        printf("P%d: No instruction found at PC %d\n", pcb->pid, pcb->programCounter);
        strcpy(pcb->state, "Finished");
        return;
    }

    // Tokenize the instruction
    char cmd[20], arg1[50], arg2[50];
    int args = sscanf(instruction, "%s %s %s", cmd, arg1, arg2);

    // Dispatch based on instruction
    if (strcmp(cmd, "assign") == 0) {
        handleAssign(pcb, arg1, arg2);
    } else if (strcmp(cmd, "print") == 0) {
        handlePrint(pcb, arg1);
    } else if (strcmp(cmd, "printFromTo") == 0) {
        handlePrintFromTo(pcb, arg1, arg2);
    } else if (strcmp(cmd, "readFile") == 0) {
        handleReadFile(pcb, arg1);
    } else if (strcmp(cmd, "writeFile") == 0) {
        handleWriteFile(pcb, arg1, arg2);
    } else if (strcmp(cmd, "semWait") == 0) {
        handleSemWait(pcb, arg1);
    } else if (strcmp(cmd, "semSignal") == 0) {
        handleSemSignal(pcb, arg1);
    } else {
        printf("P%d: Unknown instruction '%s'\n", pcb->pid, cmd);
    }
}

void handleAssign(PCB *pcb, const char* var, const char* value) {

    char key[30], val[100];
    sprintf(key, "P%d_%s", pcb->pid, var);  // e.g., "P1_x"

    const char* existing = getMemory(key);

    if(existing != NULL) {
        // Variable already exists, update its value
        if (strcmp(value, "input") == 0) {
            printf("Please enter value for %s: ", var);
            scanf("%s", val);
            setMemory(key, val);
        } else {
            setMemory(key, value);
        }
    } else {
        int slotFound = 0;
        for (int i = 1; i <= 3; i++) {
            char slotKey[30];
            sprintf(slotKey, "P%d_var%d", pcb->pid, i);
            const char* slotValue = getMemory(slotKey);

            if (strcmp(slotValue, "") == 0) {
                // Copy value into actual key like "P1_x"
                char newKey[] = sprintf("P%d_%s", pcb->pid, var);  // e.g., "P1_x"
                updateKey(slotKey, newKey);  // Update the slot key to point to the new variable
                if (strcmp(value, "input") == 0) {
                    printf("Please enter value for %s: ", var);
                    scanf("%s", val);
                    setMemory(newKey, val);
                } else {
                    setMemory(newKey, value);
                }
                slotFound = 1;
                break;
            }
        }

        if (slotFound == 0) {
            // All 3 slots are occupied, cannot assign new variable
            printf("P%d ERROR: Variable limit exceeded (3 per process)\n", pcb->pid);
        }
    }
    pcb->programCounter++;
}

handlePrint(PCB *pcb, const char* var) {
    char key[30];
    sprintf(key, "P%d_%s", pcb->pid, var);  // e.g., "P1_x"

    const char* value = getMemory(key);
    if (value != NULL) {
        printf("P%d: %s = %s\n", pcb->pid, var, value);
    } else {
        printf("P%d: Variable '%s' not found\n", pcb->pid, var);
    }
    pcb->programCounter++;
}

handlePrintFromTo(PCB *pcb, const char* startVar, const char* endVar) {
    char startKey[30], endKey[30];
    sprintf(startKey, "P%d_%s", pcb->pid, startVar);  // e.g., "P1_x"
    sprintf(endKey, "P%d_%s", pcb->pid, endVar);      // e.g., "P1_y"

    const char* startValue = getMemory(startKey);
    const char* endValue = getMemory(endKey);

    if (startValue != NULL && endValue != NULL) {
        int start = atoi(startValue);
        int end = atoi(endValue);
        for (int i = start; i <= end; i++) {
            printf("%d ", i);
        }
        printf("\n");
    } else {
        printf("P%d: One or both variables not found\n", pcb->pid);
    }
    pcb->programCounter++;
}
    