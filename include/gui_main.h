#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "process.h"

extern volatile int scheduler_wait;

// User input handling
void handle_user_input(PCB *pcb, const char* var);

// System log
void log_message(const char* message);

#endif