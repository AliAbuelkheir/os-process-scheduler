#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "process.h"


void executeInstruction(PCB *pcb);
void handleAssign(PCB *pcb, const char* var, const char* value);
void handlePrint(PCB *pcb, const char* var);
void handlePrintFromTo(PCB *pcb, const char* startVar, const char* endVar);
void handleReadFile(PCB *pcb, const char* var);
void handleAssignFromFile(PCB *pcb, const char* var, const char* sourceVar);
void handleWriteFile(PCB *pcb, const char* filenameVar, const char* contentVar);
void handleSemWait(PCB *pcb, const char* resource);
void handleSemSignal(PCB *pcb, const char* resource);

#endif