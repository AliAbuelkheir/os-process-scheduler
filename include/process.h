#ifndef PROCESS_H
#define PROCESS_H

typedef struct {
    int pid;
    char state[10];       // Ready, Running, Blocked & finished for code 
    int priority;
    int programCounter;
    int memLowerBound;
    int memUpperBound;
} PCB;

PCB createProcess(int pid, int priority, const char* filename);

void printPCB(PCB pcb);
void setState(PCB *pcb, const char *state);
const char* getState(PCB *pcb);
void setPriority(PCB *pcb, int priority);
int getPriority(int pid);
void incrementPC(PCB *pcb);
PCB* findPCBByPid(int pid);

typedef struct {
    int pid;
    int arrivalTime;
    int loaded;  
    PCB pcb;
    char sourceFile[100];
    int lastStateChange;
} ProcessInfo;

extern PCB pcbsGlobal[3];  
extern int globalPcbCount;
extern ProcessInfo processList[10];
extern int processCount;

void addProcess(const char* filename, int arrivalTime);
void sortProcessesByArrival();
ProcessInfo* findProcessByPid(int pid);


#endif