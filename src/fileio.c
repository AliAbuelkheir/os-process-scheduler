#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/fileio.h"



int readProgramFromFile(const char* filename, char* lines[]) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Failed to open file: %s\n", filename);
        return -1;
    }

    int count = 0;
    char buffer[MAX_LINE_LENGTH];

    while (fgets(buffer, sizeof(buffer), file) && count < MAX_LINES) {
        buffer[strcspn(buffer, "\n")] = '\0';

        lines[count] = strdup(buffer);
        if (!lines[count]) {
            printf("Error: Memory allocation failed for line %d\n", count);
            fclose(file);
            return -1;
        }

        count++;
    }

    fclose(file);
    printf("Read %d lines from %s\n", count, filename);
    return count; 
}
