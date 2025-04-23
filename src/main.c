#include <stdio.h>
#include <string.h>
#include "../src/process.c"

int main() {
    printf("OS Project Initialized!\n");

    PCB p1 = createPCB(1, 2, 0, 19);

    printf("PCB ID: %d\n", p1.pid);
    printf("State: %s\n", p1.state);
    printf("Priority: %d\n", p1.priority);
    printf("Memory: %d - %d\n", p1.memLowerBound, p1.memUpperBound);


    return 0;
}
