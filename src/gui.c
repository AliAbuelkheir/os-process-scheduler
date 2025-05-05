// gui.c
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "../include/scheduler.h"
#include "../include/process.h"
#include "../include/memory.h"
#include "../include/mutex.h"

#define MAX_PROCESSES 10

GtkWidget *algorithm_combo, *quantum_entry, *spin_process_count;
GtkWidget *process_entries[MAX_PROCESSES][2]; // [][0] = filename, [][1] = arrival
GtkWidget *process_form_container;
GtkWidget *log_view, *clock_label;

void append_to_log(const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1);
}

void on_load_processes(GtkButton *button, gpointer user_data) {
    int count = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_process_count));

    // Clear old entries
    GList *children = gtk_container_get_children(GTK_CONTAINER(process_form_container));
    for (GList *l = children; l != NULL; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);

    // Add new rows
    for (int i = 0; i < count && i < MAX_PROCESSES; i++) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        GtkWidget *entry_file = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry_file), "programs/Program_X.txt");
        process_entries[i][0] = entry_file;

        GtkWidget *entry_arrival = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry_arrival), "Arrival time");
        process_entries[i][1] = entry_arrival;

        gtk_box_pack_start(GTK_BOX(row), gtk_label_new("File:"), FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(row), entry_file, TRUE, TRUE, 2);
        gtk_box_pack_start(GTK_BOX(row), gtk_label_new("Arrival:"), FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(row), entry_arrival, TRUE, TRUE, 2);

        gtk_box_pack_start(GTK_BOX(process_form_container), row, FALSE, FALSE, 2);
    }
    gtk_widget_show_all(process_form_container);
}

void on_start_clicked(GtkButton *button, gpointer user_data) {
    int scheduler = gtk_combo_box_get_active(GTK_COMBO_BOX(algorithm_combo)) + 1;
    const char *quantum_text = gtk_entry_get_text(GTK_ENTRY(quantum_entry));
    int quantum = atoi(quantum_text);

    initMemory();
    initMutexes();
    initScheduler(scheduler, quantum);

    int count = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_process_count));
    for (int i = 0; i < count && i < MAX_PROCESSES; i++) {
        const char *filename = gtk_entry_get_text(GTK_ENTRY(process_entries[i][0]));
        const char *arrival_text = gtk_entry_get_text(GTK_ENTRY(process_entries[i][1]));
        int arrival = atoi(arrival_text);
        if (strlen(filename) > 0) {
            addProcess((char *)filename, arrival);
            char log_entry[256];
            snprintf(log_entry, sizeof(log_entry), "Loaded: %s at time %d\n", filename, arrival);
            append_to_log(log_entry);
        }
    }

    sortProcessesByArrival();
    append_to_log("Starting Scheduler...\n");
    startScheduler();

    char clock_text[64];
    snprintf(clock_text, sizeof(clock_text), "Clock Cycle: %d", clockCycle);
    gtk_label_set_text(GTK_LABEL(clock_label), clock_text);
    append_to_log("Simulation Finished.\n");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "OS Scheduler GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // Scheduler control
    GtkWidget *hbox_control = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), hbox_control, FALSE, FALSE, 5);

    algorithm_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "FCFS");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "MLFQ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(algorithm_combo), 0);

    quantum_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(quantum_entry), "2");
    gtk_entry_set_width_chars(GTK_ENTRY(quantum_entry), 4);

    spin_process_count = gtk_spin_button_new_with_range(1, MAX_PROCESSES, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_process_count), 3);

    GtkWidget *load_button = gtk_button_new_with_label("Load Process Inputs");
    g_signal_connect(load_button, "clicked", G_CALLBACK(on_load_processes), NULL);

    GtkWidget *start_button = gtk_button_new_with_label("Start Simulation");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(hbox_control), gtk_label_new("Algorithm:"), FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox_control), algorithm_combo, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox_control), gtk_label_new("Quantum:"), FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox_control), quantum_entry, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox_control), gtk_label_new("#Processes:"), FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox_control), spin_process_count, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox_control), load_button, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox_control), start_button, FALSE, FALSE, 2);

    // Dynamic form container
    process_form_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), process_form_container, FALSE, FALSE, 5);

    // Clock label and log
    clock_label = gtk_label_new("Clock Cycle: 0");
    gtk_box_pack_start(GTK_BOX(main_vbox), clock_label, FALSE, FALSE, 5);

    log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll), log_view);
    gtk_box_pack_start(GTK_BOX(main_vbox), scroll, TRUE, TRUE, 5);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
