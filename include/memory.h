#ifndef MEMORY_H
#define MEMORY_H

#define MEMORY_SIZE 60
#define MAX_KEY_LENGTH 20
#define MAX_VALUE_LENGTH 100

typedef struct {
    char key[MAX_KEY_LENGTH];     // Variable name, label, or metadata key
    char value[MAX_VALUE_LENGTH]; // Actual stored data (string format)
} MemoryWord;

// Global memory array
extern MemoryWord memory[MEMORY_SIZE];

// Functions
void initMemory();
int setMemory(const char* key, const char* value);
const char* getMemory(const char* key);
void printMemory();
void updateKey(const char* oldKey, const char* newKey);

int findFreeBlock(int size);


#endif
