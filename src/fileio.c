#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/fileio.h"


// returns number of instructions read
// lines[] is an array of strings, each string is a line of the program
int readProgramFromFile(const char* filename, char* lines[]) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Failed to open file: %s\n", filename);
        return -1;
    }

    int count = 0;
    char buffer[MAX_LINE_LENGTH];

    while (fgets(buffer, sizeof(buffer), file) && count < MAX_LINES) {
        // Remove newline character
        buffer[strcspn(buffer, "\n")] = '\0';

        // Duplicate line content into dynamic memory
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
    return count; // Number of instructions read
}
