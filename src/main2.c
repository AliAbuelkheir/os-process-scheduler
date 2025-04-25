#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "../include/process.h"
#include "../include/memory.h"
#include "../include/fileio.h"

int main(){

    // Initialize memory and PCB table
    initMemory();
    const char* programFiles[] = {
        "programs/Program_1.txt",
        "programs/Program_2.txt",
        "programs/Program_3.txt"
    };
    
    PCB pcbs[3];
    for (int i = 0; i < 3; i++) {
        pcbs[i] = createProcess(i + 1, i + 1, programFiles[i]);
        printPCB(pcbs[i]);
        //printf("-------------------------\n");
    }
    printMemory();

    return 0;
}