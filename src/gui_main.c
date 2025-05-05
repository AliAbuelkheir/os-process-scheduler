#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "../include/process.h"
#include "../include/memory.h"
#include "../include/scheduler.h"
#include "../include/mutex.h"

void show_dashboard(GtkWidget *window) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Overview Section
    GtkWidget *overview_label = gtk_label_new("System Overview:");
    gtk_box_pack_start(GTK_BOX(vbox), overview_label, FALSE, FALSE, 5);

    char overview[256];
    sprintf(overview, "Total Processes: %d\nClock Cycle: %d\nScheduling Algorithm: %s",
            processCount,
            clockCycle,
            schedulerType == 1 ? "FCFS" : (schedulerType == 2 ? "RR" : "MLFQ"));
    GtkWidget *overview_content = gtk_label_new(overview);
    gtk_box_pack_start(GTK_BOX(vbox), overview_content, FALSE, FALSE, 5);

    // Process List
    GtkWidget *plist_label = gtk_label_new("Process List:");
    gtk_box_pack_start(GTK_BOX(vbox), plist_label, FALSE, FALSE, 5);

    GtkWidget *plist_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(plist_view), FALSE);
    GtkTextBuffer *plist_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(plist_view));
    gtk_box_pack_start(GTK_BOX(vbox), plist_view, FALSE, FALSE, 5);

    char plist_content[2048] = "";
    for (int i = 0; i < processCount; i++) {
        PCB pcb = processList[i].pcb;
        char line[256];
        snprintf(line, sizeof(line), "PID: %d | State: %s | Priority: %d | PC: %d | Mem: %d-%d\n",
                 pcb.pid, pcb.state, pcb.priority, pcb.programCounter, pcb.memLowerBound, pcb.memUpperBound);
        strcat(plist_content, line);
    }
    gtk_text_buffer_set_text(plist_buffer, plist_content, -1);

    // Ready Queue
    GtkWidget *ready_queue_label = gtk_label_new("Ready Queue:");
    gtk_box_pack_start(GTK_BOX(vbox), ready_queue_label, FALSE, FALSE, 5);

    GtkWidget *ready_queue_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(ready_queue_view), FALSE);
    GtkTextBuffer *ready_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ready_queue_view));
    gtk_box_pack_start(GTK_BOX(vbox), ready_queue_view, FALSE, FALSE, 5);

    char ready_content[1024] = "";
    for (int i = front; i < rear; i++) {
        int pid = readyQueue[i];
        PCB *pcb = findPCBByPid(pid);
        if (pcb) {
            char line[128];
            snprintf(line, sizeof(line), "PID: %d | State: %s | PC: %d\n",
                     pcb->pid, pcb->state, pcb->programCounter);
            strcat(ready_content, line);
        }
    }
    gtk_text_buffer_set_text(ready_buffer, ready_content, -1);

    // Blocked Queues
    GtkWidget *blocked_queue_label = gtk_label_new("Blocked Queues:");
    gtk_box_pack_start(GTK_BOX(vbox), blocked_queue_label, FALSE, FALSE, 5);

    GtkWidget *blocked_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(blocked_view), FALSE);
    GtkTextBuffer *blocked_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(blocked_view));
    gtk_box_pack_start(GTK_BOX(vbox), blocked_view, FALSE, FALSE, 5);

    char blocked_content[1024] = "";
    int* queues[3] = { userInputQueue, userOutputQueue, fileQueue };
    int* counts[3] = { &inputCount, &outputCount, &fileCount };
    const char* names[3] = { "userInput", "userOutput", "file" };

    for (int q = 0; q < 3; q++) {
        if (*counts[q] > 0) {
            char title[50];
            snprintf(title, sizeof(title), "%s Queue:\n", names[q]);
            strcat(blocked_content, title);
            for (int j = 0; j < *counts[q]; j++) {
                int pid = queues[q][j];
                PCB* pcb = findPCBByPid(pid);
                if (pcb) {
                    char line[100];
                    snprintf(line, sizeof(line), "  PID: %d | PC: %d\n", pid, pcb->programCounter);
                    strcat(blocked_content, line);
                }
            }
        }
    }
    gtk_text_buffer_set_text(blocked_buffer, blocked_content, -1);

    // Running Process
    GtkWidget *running_label = gtk_label_new("Currently Running Process:");
    gtk_box_pack_start(GTK_BOX(vbox), running_label, FALSE, FALSE, 5);

    GtkWidget *running_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(running_view), FALSE);
    GtkTextBuffer *running_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(running_view));
    gtk_box_pack_start(GTK_BOX(vbox), running_view, FALSE, FALSE, 5);

    char running_content[256] = "";
    for (int i = 0; i < processCount; i++) {
        PCB* pcb = &processList[i].pcb;
        if (strcmp(pcb->state, "Running") == 0) {
            char key[30];
            sprintf(key, "P%d_line_%d", pcb->pid, pcb->programCounter);
            const char* instr = getMemory(key);
            snprintf(running_content, sizeof(running_content),
                     "PID: %d\nCurrent Instruction: %s\nProgram Counter: %d",
                     pcb->pid, instr ? instr : "N/A", pcb->programCounter);
            break;
        }
    }
    gtk_text_buffer_set_text(running_buffer, running_content, -1);

    gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "OS Scheduler Dashboard");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    show_dashboard(window);

    gtk_main();
    return 0;
}
