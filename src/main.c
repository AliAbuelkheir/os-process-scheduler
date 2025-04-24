#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/process.c"

#define MEMORY_SIZE 60
char* memory[MEMORY_SIZE] = {NULL}; 
#define MAX_LINES 15
#define MAX_LINE_LENGTH 100
#define MAX_PROCESSES 5
#define MAX_FILES 10
char* fileNames[MAX_FILES] = {NULL};
char* fileContents[MAX_FILES] = {NULL};
PCB pcbTable[MAX_PROCESSES];
int pcbCount = 0;

int allocateMemory(char* codeLines[], int numLines, int pid, PCB* pcbOut) {
    int totalRequired = 4 + 3 + numLines;  // PCB + variables + code
    int start = -1; 

    for (int i = 0; i <= MEMORY_SIZE - totalRequired; i++) {
        int available = 1;
        for (int j = 0; j < totalRequired; j++) {
            if (memory[i + j] != NULL) {
                available = 0;
                break;
            }
        }
        if (available) {
            start = i;
            break;
        }
    }

    if (start == -1) {
        printf("Not enough memory for process %d\n", pid);
        return -1;
    }

    memory[start] = strdup("State=Ready");
    memory[start + 1] = malloc(20); 
    sprintf(memory[start + 1], "PID=%d", pid);
    memory[start + 2] = strdup("PC=0");
    memory[start + 3] = strdup("Priority=1");

    memory[start + 4] = strdup("a=");
    memory[start + 5] = strdup("b=");
    memory[start + 6] = strdup("c=");    

    for (int i = 0; i < numLines; i++) {
        memory[start + 7 + i] = strdup(codeLines[i]);
    }

    pcbOut->pid = pid;
    strcpy(pcbOut->state, "Ready");
    pcbOut->priority = 1;
    pcbOut->programCounter = 0;
    pcbOut->memLowerBound = start;
    pcbOut->memUpperBound = start + totalRequired - 1;

    return 0;
}

int readProgramFromFile(const char* filename, char* lines[]) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("failed to open file: %s\n", filename);
        return -1;
    }

    int count = 0;
    char buffer[MAX_LINE_LENGTH];

    while (fgets(buffer, sizeof(buffer), file) && count < MAX_LINES) {
        buffer[strcspn(buffer, "\n")] = 0; 
        lines[count] = strdup(buffer);     
        count++;
    }

    fclose(file);
    return count; 
}

int userInput = -1, userOutput = -1, file = -1;
int userInputQueue[MAX_PROCESSES]; int userOutputQueue[MAX_PROCESSES]; int fileQueue[MAX_PROCESSES];
int inputCount = 0, outputCount = 0, fileCount = 0;

void enqueue(int queue[], int* count, int pid) {
    if (*count < MAX_PROCESSES) {
        queue[*count] = pid;
        (*count)++;
    }
}

int dequeue(int queue[], int* count) {
    if (*count == 0) return -1;
    int pid = queue[0];
    for (int i = 1; i < *count; i++) queue[i - 1] = queue[i];
    (*count)--;
    return pid;
}

int semWait(PCB* pcb, const char* resource) {
    if (strcmp(resource, "userInput") == 0) {
        if (userInput == -1) {
            userInput = pcb->pid;
            return 1;
        } 
        else {
            enqueue(userInputQueue, &inputCount, pcb->pid);
            setState(pcb, "Blocked");
            return 0;
        }
    }
    if (strcmp(resource, "userOutput") == 0) {
        if (userOutput == -1) {
            userOutput = pcb->pid;
            return 1;
        } 
        else {
            enqueue(userOutputQueue, &outputCount, pcb->pid);
            setState(pcb, "Blocked");
            return 0;
        }
    }
    if (strcmp(resource, "file") == 0) {
        if (file == -1) {
            file = pcb->pid;
            return 1;
        } 
        else {
            enqueue(fileQueue, &fileCount, pcb->pid);
            setState(pcb, "Blocked");
            return 0;
        }
    }
    return 1;
}

void semSignal(const char* resource) {
    if (strcmp(resource, "userInput") == 0) {
        if (inputCount > 0) {
            int nextPid = dequeue(userInputQueue, &inputCount);
            for (int i = 0; i < pcbCount; i++) {
                if (pcbTable[i].pid == nextPid) {
                    setState(&pcbTable[i], "Ready");
                    break;
                }
            }
            userInput = nextPid;
        } else {
            userInput = -1;
        }
    }
    if (strcmp(resource, "userOutput") == 0) {
        if (outputCount > 0) {
            int nextPid = dequeue(userOutputQueue, &outputCount);
            for (int i = 0; i < pcbCount; i++) {
                if (pcbTable[i].pid == nextPid) {
                    setState(&pcbTable[i], "Ready");
                    break;
                }
            }
            userOutput = nextPid;
        } else {
            userOutput = -1;
        }
    }
    if (strcmp(resource, "file") == 0) {
        if (fileCount > 0) {
            int nextPid = dequeue(fileQueue, &fileCount);
            for (int i = 0; i < pcbCount; i++) {
                if (pcbTable[i].pid == nextPid) {
                    setState(&pcbTable[i], "Ready");
                    break;
                }
            }
            file = nextPid;
        } else {
            file = -1;
        }
    }
}

char* getVariable(PCB* pcb, const char* name) {
    static char value[100];
    int start = pcb->memLowerBound + 4;
    for (int i = 0; i < 3; i++) {
        if (memory[start + i]) {
            char varName[20], varValue[100];
            if (sscanf(memory[start + i], "%[^=]=%s", varName, varValue) == 2) {
                if (strcmp(varName, name) == 0) {
                    strcpy(value, varValue);
                    return value;
                }
            }
        }
    }
    return NULL;
}

void saveVariable(PCB* pcb, const char* line) {
    char name[20];
    sscanf(line, "%[^=]", name);
    int start = pcb->memLowerBound + 4;

    for (int i = 0; i < 3; i++) {
        if (memory[start + i]) {
            char varName[20];
            sscanf(memory[start + i], "%[^=]", varName);
            if (strcmp(varName, name) == 0) {
                free(memory[start + i]);
                memory[start + i] = strdup(line);
                return;
            }
        }
    }
    for (int i = 0; i < 3; i++) {
        if (!memory[start + i]) {
            memory[start + i] = strdup(line);
            return;
        }
    }

    free(memory[start]);
    memory[start] = strdup(line);
}


void writeFile(const char* filename, const char* content) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fileNames[i] && strcmp(fileNames[i], filename) == 0) {
            free(fileContents[i]);
            fileContents[i] = strdup(content);
            return;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (!fileNames[i]) {
            fileNames[i] = strdup(filename);
            fileContents[i] = strdup(content);
            return;
        }
    }
    printf("writeFile ERROR: Max file limit reached\n");
}

char* readFile(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fileNames[i] && strcmp(fileNames[i], filename) == 0) {
            return fileContents[i];
        }
    }
    return "[file not found]";
}

void executeInstruction(PCB* pcb) {
    int pc = pcb->programCounter;
    int codeStart = pcb->memLowerBound + 7;
    char* instruction = memory[codeStart + pc];

    printf("[PID %d] Executing: %s\n", pcb->pid, instruction);

    if (!instruction) return;
    
    // clear buffers to avoid garbage
    char cmd[20] = "", arg1[20] = "", arg2[20] = "";
    int parts = sscanf(instruction, "%s %s %s", cmd, arg1, arg2);
    if (parts < 1) return;    

    // semWait
    if (strcmp(cmd, "semWait") == 0) {
        if (!semWait(pcb, arg1)) return;
    }

    // semSignal
    else if (strcmp(cmd, "semSignal") == 0) {
        semSignal(arg1);
    }

    // assign x input
    else if (strcmp(cmd, "assign") == 0 && strcmp(arg2, "input") == 0) {
        if (userInput == pcb->pid && strcmp(pcb->state, "Blocked") != 0) {
            printf("Please enter value for %s: ", arg1);
            char val[100]; 
            scanf("%s", val);
            char assignment[120];
            sprintf(assignment, "%s=%s", arg1, val);
            saveVariable(pcb, assignment);
        } 
        else {
            printf("PID %d attempted input without userInput lock\n", pcb->pid);
        }
    }

    // assign x readFile y
    else if (strcmp(cmd, "assign") == 0 && strcmp(arg2, "readFile") == 0) {
        char nextArg[20] = "";
        sscanf(instruction, "%*s %*s %*s %s", nextArg);
        if (strlen(nextArg) == 0) {
            printf("Missing file name for readFile.\n");
            return;
        }
        char* fileName = getVariable(pcb, nextArg);
        printf("Reading from file named: %s\n", fileName);
        char* val = readFile(fileName ? fileName : nextArg);
        char assignment[120];
        sprintf(assignment, "%s=%s", arg1, val);
        saveVariable(pcb, assignment);
    }

    // assign x y (normal or from another variable)
    else if (strcmp(cmd, "assign") == 0) {
        char assignment[120];
        sprintf(assignment, "%s=%s", arg1, arg2);
        saveVariable(pcb, assignment);
    }

    // writeFile x y
    else if (strcmp(cmd, "writeFile") == 0) {
        char* fileName = getVariable(pcb, arg1);
        char* fileContent = getVariable(pcb, arg2);
        writeFile(fileName ? fileName : arg1, fileContent ? fileContent : arg2);
    }

    // print x
    else if (strcmp(cmd, "print") == 0) {
        if (userOutput == pcb->pid) {
            char* val = getVariable(pcb, arg1);
            if (val != NULL) {
                printf("%s\n", val);
            } 
            else {
                printf("[undefined]\n");
            }
        } 
        else {
            printf("PID %d attempted output without userOutput lock\n", pcb->pid);
        }
    }

    // printFromTo x y
    else if (strcmp(cmd, "printFromTo") == 0) {
        if (userOutput == pcb->pid) {
            char* valA = getVariable(pcb, arg1);
            char* valB = getVariable(pcb, arg2);
    
            if (valA == NULL || valB == NULL) {
                printf("Error: Variable(s) not found for printFromTo (%s, %s)\n", arg1, arg2);
            } else {
                int a = atoi(valA);
                int b = atoi(valB);
                for (int i = a; i <= b; i++) {
                    printf("%d ", i);
                }
                printf("\n");
            }
        } 
        else {
            printf("PID %d attempted printFromTo without userOutput lock\n", pcb->pid);
        }
    }    

    pcb->programCounter++;
}

PCB* findPCB(int pid) {
    for (int i = 0; i < pcbCount; i++) {
        if (pcbTable[i].pid == pid)
            return &pcbTable[i];
    }
    return NULL;
}

int readyQueue[MAX_PROCESSES];
int readyCount = 0;

void addToReadyQueue(int pid) {
    readyQueue[readyCount++] = pid;
}

int popFromReadyQueue() {
    if (readyCount == 0) return -1;
    int pid = readyQueue[0];
    for (int i = 1; i < readyCount; i++)
        readyQueue[i - 1] = readyQueue[i];
    readyCount--;
    return pid;
}

void runFCFS() {
    while (readyCount > 0) {
        int pid = popFromReadyQueue();
        PCB* pcb = findPCB(pid);
        while (strcmp(pcb->state, "Blocked") != 0 && pcb->programCounter <= pcb->memUpperBound - (pcb->memLowerBound + 7)) {
            executeInstruction(pcb);
        }
    }
}

void refreshReadyQueue() {
    for (int i = 0; i < pcbCount; i++) {
        int alreadyQueued = 0;
        for (int j = 0; j < readyCount; j++) {
            if (readyQueue[j] == pcbTable[i].pid) {
                alreadyQueued = 1;
                break;
            }
        }

        if (!alreadyQueued &&
            strcmp(pcbTable[i].state, "Ready") == 0 &&
            pcbTable[i].programCounter <= pcbTable[i].memUpperBound - (pcbTable[i].memLowerBound + 7)) {
            addToReadyQueue(pcbTable[i].pid);
        }
    }
}

void runRoundRobin(int quantum) {
    while (readyCount > 0) {
        int pid = popFromReadyQueue();
        PCB* pcb = findPCB(pid);
        int executed = 0;

        while (executed < quantum &&
               strcmp(pcb->state, "Blocked") != 0 &&
               pcb->programCounter <= pcb->memUpperBound - (pcb->memLowerBound + 7)) {
            executeInstruction(pcb);
            executed++;
        }

        if (strcmp(pcb->state, "Ready") == 0 &&
            pcb->programCounter <= pcb->memUpperBound - (pcb->memLowerBound + 7)) {
            addToReadyQueue(pcb->pid);
        }
        refreshReadyQueue();
    }
}


int queues[4][MAX_PROCESSES];
int queueCounts[4] = {0, 0, 0, 0};
int quantums[4] = {1, 2, 4, 8};

void addToMLFQ(int level, int pid) {
    queues[level][queueCounts[level]++] = pid;
}

int queuesNotEmpty() {
    return queueCounts[0] || queueCounts[1] || queueCounts[2] || queueCounts[3];
}

void runMLFQ() {
    while (queuesNotEmpty()) {
        for (int level = 0; level < 4; level++) {
            if (queueCounts[level] == 0) continue;

            int pid = queues[level][0];
            for (int i = 1; i < queueCounts[level]; i++)
                queues[level][i - 1] = queues[level][i];
            queueCounts[level]--;

            PCB* pcb = findPCB(pid);
            int quantum = quantums[level];
            int executed = 0;

            while (executed < quantum && strcmp(pcb->state, "Blocked") != 0 && pcb->programCounter <= pcb->memUpperBound - (pcb->memLowerBound + 7)) {
                executeInstruction(pcb);
                executed++;
            }

            if (strcmp(pcb->state, "Ready") == 0 && pcb->programCounter <= pcb->memUpperBound - (pcb->memLowerBound + 7)) {
                if (level < 3) addToMLFQ(level + 1, pid);
                else addToMLFQ(level, pid); // RR in level 4
            }
        }
    }
}

void cleanupMemory() {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i]) {
            free(memory[i]);
            memory[i] = NULL;
        }
    }
    for (int i = 0; i < MAX_FILES; i++) {
        free(fileNames[i]);
        free(fileContents[i]);
        fileNames[i] = NULL;
        fileContents[i] = NULL;
    }
}

int main() {
    printf("OS Project Initialized!\n");

    const char* programFiles[] = {
        "Program_1.txt",
        "Program_2.txt",
        "Program_3.txt"
    };

    const int numPrograms = 3;

    for (int i = 0; i < numPrograms; i++) {
        printf("\n=== Loading %s ===\n", programFiles[i]);

        char* codeLines[MAX_LINES];
        int lineCount = readProgramFromFile(programFiles[i], codeLines);

        if (lineCount <= 0) {
            printf("Skipping %s due to read error.\n", programFiles[i]);
            continue;
        }

        PCB pcb;
        int allocResult = allocateMemory(codeLines, lineCount, i + 1, &pcb);

        if (allocResult != 0) {
            printf("Skipping %s due to memory allocation failure.\n", programFiles[i]);
            continue;
        }

        pcbTable[pcbCount++] = pcb;

        printf("Created PCB for PID %d\n", pcb.pid);
        printf("Memory allocated from %d to %d\n", pcb.memLowerBound, pcb.memUpperBound);

        for (int j = pcb.memLowerBound; j <= pcb.memUpperBound; j++) {
            printf("  memory[%d] = %s\n", j, memory[j]);
        }

        for (int k = 0; k < lineCount; k++) {
            free(codeLines[k]);
        }
    }

    int choice, quantum;
    printf("\nChoose a scheduling algorithm:\n");
    printf("1. First-Come First-Serve (FCFS)\n");
    printf("2. Round Robin (RR)\n");
    printf("3. Multilevel Feedback Queue (MLFQ)\n");
    printf("Enter choice: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            for (int i = 0; i < pcbCount; i++)
                addToReadyQueue(pcbTable[i].pid);
            runFCFS();
            break;
        case 2:
            printf("Enter quantum: ");
            scanf("%d", &quantum);
            for (int i = 0; i < pcbCount; i++)
                addToReadyQueue(pcbTable[i].pid);
            runRoundRobin(quantum);
            break;
        case 3:
            for (int i = 0; i < pcbCount; i++)
                addToMLFQ(0, pcbTable[i].pid);
            runMLFQ();
            break;
        default:
            printf("Invalid choice. Exiting...\n");
            break;
    }

    printf("\n=== Final Memory Dump ===\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i]) {
            printf("  memory[%d] = %s\n", i, memory[i]);
        }
    }

    printf("\n=== File System ===\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (fileNames[i]) {
            printf("  %s = %s\n", fileNames[i], fileContents[i]);
        }
    }

    printf("\n=== Mutex Final States ===\n");
    printf("userInput lock held by: %d\n", userInput);
    printf("userOutput lock held by: %d\n", userOutput);
    printf("file lock held by: %d\n", file);

    cleanupMemory();
    return 0;
}
