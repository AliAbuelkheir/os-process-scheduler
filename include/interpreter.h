#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "process.h"

typedef void (*InputHandler)(PCB*, const char*);

int executeInstruction(PCB *pcb, int clockCycle);
void handleAssign(PCB *pcb, const char* var, const char* value);
int isStringNumber(const char* str);
void handlePrint(PCB *pcb, const char* var);
void handlePrintFromTo(PCB *pcb, const char* startVar, const char* endVar);
void handleReadFile(PCB *pcb, const char* var);
void handleAssignFromFile(PCB *pcb, const char* var, const char* sourceVar);
void handleWriteFile(PCB *pcb, const char* filenameVar, const char* contentVar);
//void handleSemWait(PCB *pcb, const char* resource);
void handleSemSignal(PCB *pcb, const char* resource);
const char* getCurrentInstruction(PCB *pcb);

void setInputHandler(InputHandler handler);


#endif