// gui_control.c
#include <gtk/gtk.h>
#include "../gui/gui_control.h"
#include "../include/scheduler.h"
#include "../include/process.h"
#include "../include/memory.h"
#include "../include/mutex.h"

GtkWidget *quantum_entry;
GtkWidget *program_combo;
GtkWidget *arrival_entry;
GtkWidget *process_list_box;
GtkWidget *algorithm_combo;
GtkWidget *start_button, *stop_button, *reset_button;

static void on_algorithm_changed(GtkComboBox *combo, gpointer user_data) {
    const gchar *algo = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    gtk_widget_set_visible(quantum_entry, g_strcmp0(algo, "Round Robin") == 0);
}

static void on_add_process(GtkButton *button, gpointer user_data) {
    const gchar *selected_program = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(program_combo));
    const gchar *arrival_time = gtk_entry_get_text(GTK_ENTRY(arrival_entry));

    if (!selected_program || !arrival_time || strlen(arrival_time) == 0) return;

    char *filename;
    if (g_strcmp0(selected_program, "Program 1 - CPU Bound") == 0)
        filename = "programs/Program_1.txt";
    else if (g_strcmp0(selected_program, "Program 2 - I/O Bound") == 0)
        filename = "programs/Program_2.txt";
    else
        filename = "programs/Program_3.txt";

    int arrival = atoi(arrival_time);
    addProcess(filename, arrival);

    // Add to GUI List
    gchar *label_text = g_strdup_printf("%s (Arrival: %d)", selected_program, arrival);
    GtkWidget *label = gtk_label_new(label_text);
    gtk_box_pack_start(GTK_BOX(process_list_box), label, FALSE, FALSE, 2);
    gtk_widget_show(label);
    g_free(label_text);
}

static void on_start_simulation(GtkButton *button, gpointer user_data) {
    const gchar *algo = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(algorithm_combo));
    int scheduler = 1;
    int quantum = 2;

    if (g_strcmp0(algo, "Round Robin") == 0) {
        scheduler = 2;
        quantum = atoi(gtk_entry_get_text(GTK_ENTRY(quantum_entry)));
    } else if (g_strcmp0(algo, "Multilevel Feedback Queue") == 0) {
        scheduler = 3;
    }

    initMemory();
    initMutexes();
    initScheduler(scheduler, quantum);
    sortProcessesByArrival();
    startScheduler();
}

GtkWidget* gui_create_control_panel() {
    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *frame = gtk_frame_new("Scheduler Control Panel");
    gtk_container_add(GTK_CONTAINER(frame), panel);

    // Algorithm selector
    algorithm_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "First Come First Serve");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "Multilevel Feedback Queue");
    gtk_combo_box_set_active(GTK_COMBO_BOX(algorithm_combo), 0);
    g_signal_connect(algorithm_combo, "changed", G_CALLBACK(on_algorithm_changed), NULL);

    GtkWidget *algo_label = gtk_label_new("Scheduling Algorithm:");
    gtk_box_pack_start(GTK_BOX(panel), algo_label, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(panel), algorithm_combo, FALSE, FALSE, 2);

    // Quantum input
    quantum_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(quantum_entry), "Quantum for RR");
    gtk_widget_set_visible(quantum_entry, FALSE);
    gtk_box_pack_start(GTK_BOX(panel), quantum_entry, FALSE, FALSE, 2);

    // Process dropdown
    program_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(program_combo), "Program 1 - CPU Bound");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(program_combo), "Program 2 - I/O Bound");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(program_combo), "Program 3 - Mixed");
    gtk_combo_box_set_active(GTK_COMBO_BOX(program_combo), 0);

    arrival_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(arrival_entry), "Arrival Time");

    GtkWidget *add_button = gtk_button_new_with_label("Add Process");
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_process), NULL);

    gtk_box_pack_start(GTK_BOX(panel), gtk_label_new("Choose a Program:"), FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(panel), program_combo, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(panel), arrival_entry, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(panel), add_button, FALSE, FALSE, 2);

    // Process List
    process_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll, 250, 150);
    gtk_container_add(GTK_CONTAINER(scroll), process_list_box);
    gtk_box_pack_start(GTK_BOX(panel), gtk_label_new("Loaded Processes:"), FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(panel), scroll, FALSE, FALSE, 2);

    // Start Button
    start_button = gtk_button_new_with_label("Start Simulation");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_simulation), NULL);
    gtk_box_pack_start(GTK_BOX(panel), start_button, FALSE, FALSE, 5);

    return frame;
}
