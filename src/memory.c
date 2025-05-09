#include <stdio.h>
#include <string.h>
#include "../include/memory.h"

MemoryWord memory[MEMORY_SIZE];

void initMemory() {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i].key[0] = '\0';   // Mark as empty
        memory[i].value[0] = '\0';
    }
}

int setMemory(const char* key, const char* value) {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (strcmp(memory[i].key, key) == 0) {
            strcpy(memory[i].value, value);
            return i;
        }
    }

    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i].key[0] == '\0') {
            strcpy(memory[i].key, key);
            strcpy(memory[i].value, value);
            return i;
        }
    }

    printf("Memory full, cannot store key: %s\n", key);
    return -1;
}

const char* getMemory(const char* key) {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (strcmp(memory[i].key, key) == 0) {
            return memory[i].value;
        }
    }
    return NULL;
}

void printMemory() {
    printf("--------- Memory State ---------\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i].key[0] != '\0') {
            printf("[%2d] %s : %s\n", i, memory[i].key, memory[i].value);
        }
    }
    printf("--------------------------------\n");
}

int findFreeBlock(int size) {
    int freeCount = 0;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i].key[0] == '\0') {
            freeCount++;
            if (freeCount == size) {
                return i - size + 1; // start index
            }
        } else {
            freeCount = 0;
        }
    }
    return -1;  // not enough space
}

void updateKey(const char* oldKey, const char* newKey) {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (strcmp(memory[i].key, oldKey) == 0) {
            strcpy(memory[i].key, newKey);
            return;
        }
    }
    printf("Key %s not found in memory.\n", oldKey);
}