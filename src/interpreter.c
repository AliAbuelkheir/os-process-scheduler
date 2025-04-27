#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/interpreter.h"
#include "../include/memory.h"
#include "../include/process.h"
#include "../include/fileio.h"
#include "../include/mutex.h"

void executeInstruction(PCB *pcb) {
    char key[30];
    sprintf(key, "P%d_line_%d", pcb->pid, pcb->programCounter);

    const char* instruction = getMemory(key);
    if (instruction == NULL || strcmp(instruction, "") == 0) {
        strcpy(pcb->state, "Finished");
        return;
    }

    // Tokenize the instruction
    char cmd[20], arg1[50], arg2[50], arg3[50];
    // int args = sscanf(instruction, "%s %s %s", cmd, arg1, arg2);
    int args = sscanf(instruction, "%s %s %s %s", cmd, arg1, arg2, arg3);

    // Dispatch based on instruction
    if (strcmp(cmd, "assign") == 0) {
        if (strcmp(arg2, "readFile") == 0) {
            handleAssignFromFile(pcb, arg1, arg3);  // special case
        } else {
            handleAssign(pcb, arg1, arg2);
        }
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

// void handleAssign(PCB *pcb, const char* var, const char* value) {

//     char key[30], val[100];
//     sprintf(key, "P%d_%s", pcb->pid, var);  // e.g., "P1_x"

//     const char* existing = getMemory(key);

//     if(existing != NULL) {
//         // Variable already exists, update its value
//         if (strcmp(value, "input") == 0) {
//             printf("Please enter value for %s: ", var);
//             scanf("%s", val);
//             setMemory(key, val);
//         } else {
//             setMemory(key, value);
//         }
//     } else {
//         int slotFound = 0;
//         for (int i = 1; i <= 3; i++) {
//             char slotKey[30];
//             sprintf(slotKey, "P%d_var%d", pcb->pid, i);
//             const char* slotValue = getMemory(slotKey);

//             if (strcmp(slotValue, "") == 0) {
//                 // Copy value into actual key like "P1_x"
//                 // char newKey[] = sprintf("P%d_%s", pcb->pid, var);  // e.g., "P1_x"
//                 char newKey[30];
//                 sprintf(newKey, "P%d_%s", pcb->pid, var);
//                 updateKey(slotKey, newKey);  // Update the slot key to point to the new variable
//                 if (strcmp(value, "input") == 0) {
//                     printf("Please enter value for %s: ", var);
//                     scanf("%s", val);
//                     setMemory(newKey, val);
//                 } else {
//                     setMemory(newKey, value);
//                 }
//                 slotFound = 1;
//                 break;
//             }
//         }

//         if (slotFound == 0) {
//             // All 3 slots are occupied, cannot assign new variable
//             printf("P%d ERROR: Variable limit exceeded (3 per process)\n", pcb->pid);
//         }
//     }
//     pcb->programCounter++;
// }

void handleAssign(PCB *pcb, const char* var, const char* value) {
    char key[30], val[100];
    sprintf(key, "P%d_%s", pcb->pid, var);  // e.g., "P1_b"

    const char* existing = getMemory(key);

    if (existing != NULL) {
        if (strcmp(value, "input") == 0) {
            printf("Please enter value for %s: ", var);
            scanf("%s", val);
            setMemory(key, val);
        } 
        else {
            setMemory(key, value);
        }
    } 
    else {
        int slotFound = 0;
        for (int i = 1; i <= 3; i++) {
            char slotKey[30];
            sprintf(slotKey, "P%d_var%d", pcb->pid, i);
            const char* slotValue = getMemory(slotKey);

            if (slotValue && strcmp(slotValue, "") == 0) {
                setMemory(slotKey, var);  // Save the variable name

                if (strcmp(value, "input") == 0) {
                    printf("Please enter value for %s: ", var);
                    scanf("%s", val);
                    sprintf(key, "P%d_%s", pcb->pid, var);
                    setMemory(key, val);
                } 
                else {
                    sprintf(key, "P%d_%s", pcb->pid, var);
                    setMemory(key, value);
                }

                slotFound = 1;
                break;
            }
        }

        if (slotFound == 0) {
            printf("P%d ERROR: Variable limit exceeded (3 per process)\n", pcb->pid);
        }
    }
    incrementPC(pcb);
}

void handlePrint(PCB *pcb, const char* var) {
    char key[30];
    sprintf(key, "P%d_%s", pcb->pid, var);  // e.g., "P1_x"

    const char* value = getMemory(key);
    if (value != NULL) {
        printf("P%d: %s = %s\n", pcb->pid, var, value);
    } else {
        printf("P%d: Variable '%s' not found\n", pcb->pid, var);
    }
    incrementPC(pcb);
}

void handlePrintFromTo(PCB *pcb, const char* startVar, const char* endVar) {
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
    incrementPC(pcb);
}

void handleReadFile(PCB *pcb, const char* var) {
    char key[30];
    sprintf(key, "P%d_%s", pcb->pid, var);

    const char* filename = getMemory(key);
    if (filename == NULL) {
        printf("P%d: Filename variable '%s' not found\n", pcb->pid, var);
        pcb->programCounter++;
        return;
    }

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("P%d: Cannot open file '%s' for reading\n", pcb->pid, filename);
        pcb->programCounter++;
        return;
    }

    char buffer[100];
    if (fgets(buffer, sizeof(buffer), file)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        setMemory(key, buffer);
    } else {
        printf("P%d: File '%s' is empty\n", pcb->pid, filename);
    }
    fclose(file);
    incrementPC(pcb);
}

void handleAssignFromFile(PCB *pcb, const char* var, const char* sourceVar) {
    char filenameKey[30];
    sprintf(filenameKey, "P%d_%s", pcb->pid, sourceVar);
    const char* filename = getMemory(filenameKey);

    if (filename == NULL) {
        printf("P%d: Filename variable '%s' not found\n", pcb->pid, sourceVar);
        pcb->programCounter++;
        return;
    }

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("P%d: Cannot open file '%s'\n", pcb->pid, filename);
    } else {
        char buffer[100];
        if (fgets(buffer, sizeof(buffer), file)) {
            buffer[strcspn(buffer, "\n")] = '\0';
            char varKey[30];
            sprintf(varKey, "P%d_%s", pcb->pid, var);
            setMemory(varKey, buffer);
        }
        fclose(file);
    }
    incrementPC(pcb);
}


void handleWriteFile(PCB *pcb, const char* filenameVar, const char* contentVar) {
    char filenameKey[30], contentKey[30];
    sprintf(filenameKey, "P%d_%s", pcb->pid, filenameVar);
    sprintf(contentKey, "P%d_%s", pcb->pid, contentVar);

    const char* filename = getMemory(filenameKey);
    const char* content = getMemory(contentKey);

    if (filename == NULL || content == NULL) {
        printf("P%d: Error writing file: missing filename or content\n", pcb->pid);
        pcb->programCounter++;
        return;
    }

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("P%d: Failed to open file '%s' for writing\n", pcb->pid, filename);
    } else {
        fprintf(file, "%s\n", content);
        fclose(file);
    }
    incrementPC(pcb);
}

void handleSemWait(PCB *pcb, const char* resource) {
    if (!semWait(pcb, resource)) {
        printf("P%d: Blocked while waiting for %s\n", pcb->pid, resource);
        return;
    }
    incrementPC(pcb);
}

void handleSemSignal(PCB *pcb, const char* resource) {
    semSignal(resource);
    incrementPC(pcb);
}