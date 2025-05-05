#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/memory.h"
#include "../include/process.h"
#include "../include/scheduler.h"
#include "../include/mutex.h"
#include "../include/fileio.h"
#include "../include/interpreter.h"

// Backend initialization functions
void initMemory();
void initMutexes();

// Forward declarations for panel initialization functions
GtkWidget* init_control_panel(GtkWidget *proc_config_panel);
GtkWidget* init_process_config_panel();
GtkWidget* init_process_dashboard();
GtkWidget* init_memory_viewer();
GtkWidget* init_log_panel();

// Global variables to track application state
GtkWidget *g_process_treeview;
GtkWidget *g_log_textview;
GtkWidget *g_memory_grid;
gboolean scheduler_running = FALSE;
guint timeout_id = 0;

void log_message(const char *message) {
    GtkTextBuffer *buffer;
    GtkTextIter end;
    
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g_log_textview));
    gtk_text_buffer_get_end_iter(buffer, &end);
    
    // Add newline if buffer is not empty
    if (gtk_text_buffer_get_char_count(buffer) > 0) {
        gtk_text_buffer_insert(buffer, &end, "\n", -1);
    }
    
    gtk_text_buffer_insert(buffer, &end, message, -1);
    
    // Scroll to bottom
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(g_log_textview), &end, 0.0, FALSE, 0.0, 0.0);
}

void update_process_dashboard() {
    GtkListStore *store;
    GtkTreeIter iter;
    
    // Get the list store from treeview
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(g_process_treeview)));
    gtk_list_store_clear(store);
    
    // Update with current processes
    for (int i = 0; i < processCount; i++) {
        ProcessInfo *info = &processList[i];
        PCB *pcb = &info->pcb;
        
        if (info->loaded) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                              0, pcb->pid,
                              1, pcb->state,
                              2, pcb->priority,
                              3, pcb->programCounter,
                              4, pcb->memLowerBound,
                              5, pcb->memUpperBound,
                              -1);
        }
    }
}

void update_memory_view() {
    GtkWidget *value_label;
    GtkWidget *key_label;
    char buffer[128];
    
    for (int i = 0; i < MEMORY_SIZE; i++) {
        // Get the value label from grid
        GtkWidget *child = gtk_grid_get_child_at(GTK_GRID(g_memory_grid), 
                                               (i % 2) * 2 + 1, // column
                                               (i / 2) + 1);    // row
        
        if (child && memory[i].key[0] != '\0') {
            sprintf(buffer, "%.10s: %.10s", memory[i].key, memory[i].value);
            gtk_label_set_text(GTK_LABEL(child), buffer);
        } else if (child) {
            gtk_label_set_text(GTK_LABEL(child), "(empty)");
        }
    }
}

// Callback for when scheduler type changes
void on_scheduler_type_changed(GtkComboBox *combo_box, gpointer user_data) {
    GtkWidget *quantum_label = GTK_WIDGET(user_data);
    GtkWidget *quantum_spin = GTK_WIDGET(g_object_get_data(G_OBJECT(quantum_label), "quantum_spin"));
    
    int active = gtk_combo_box_get_active(combo_box);
    
    // Show/hide quantum UI based on scheduler type
    if (active == 1) { // Round Robin
        gtk_widget_set_visible(quantum_label, TRUE);
        gtk_widget_set_visible(quantum_spin, TRUE);
    } else {
        gtk_widget_set_visible(quantum_label, FALSE);
        gtk_widget_set_visible(quantum_spin, FALSE);
    }
}

// Callback for when add process button is clicked
void on_add_process_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *file_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "file_entry"));
    GtkWidget *arrival_spin = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "arrival_spin"));
    
    const char *filename = gtk_entry_get_text(GTK_ENTRY(file_entry));
    int arrival_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(arrival_spin));
    
    if (strlen(filename) > 0) {
        addProcess(filename, arrival_time);
        
        // Add to visual process list
        GtkWidget *process_list = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "process_list"));
        char buffer[256];
        sprintf(buffer, "PID %d: %s (arrival: %d)", processCount, filename, arrival_time);
        
        GtkWidget *row = gtk_label_new(buffer);
        gtk_widget_set_halign(row, GTK_ALIGN_START);
        gtk_list_box_insert(GTK_LIST_BOX(process_list), row, -1);
        gtk_widget_show(row);
        
        char log_msg[300];
        sprintf(log_msg, "Added process from file %s with arrival time %d", filename, arrival_time);
        log_message(log_msg);
    }
}

// Function to perform one scheduler step
gboolean scheduler_step(gpointer user_data) {
    // Check if we should stop
    if (!scheduler_running) {
        timeout_id = 0;
        return FALSE; // stop the timer
    }
    
    // Load arrived processes
    for (int i = 0; i < processCount; i++) {
        if (processList[i].arrivalTime <= clockCycle && !processList[i].loaded) {
            processList[i].loaded = 1;
            PCB *pcb = &processList[i].pcb;
            
            if (schedulerType == 3) {
                setState(pcb, "Ready");
                addToMLFQ(pcb->pid, 0);
            } else {
                setState(pcb, "Ready");
                addToReadyQueue(pcb->pid);
            }
            
            char log_msg[100];
            sprintf(log_msg, "Process %d loaded at clock cycle %d", pcb->pid, clockCycle);
            log_message(log_msg);
        }
    }
    
    // Run one instruction based on scheduler type
    int pid = -1;
    
    if (schedulerType == 1) { // FCFS
        if (front != rear) {
            pid = readyQueue[front++];
        }
    } else if (schedulerType == 2) { // RR
        // Simple RR implementation for GUI
        if (front != rear) {
            pid = readyQueue[front++];
            if (pid != -1) {
                // Re-add to end of queue if not finished after execution
                PCB *pcb = findPCBByPid(pid);
                if (pcb && strcmp(pcb->state, "Ready") == 0) {
                    readyQueue[rear++] = pid;
                }
            }
        }
    } else { // MLFQ
        // Simplified MLFQ for GUI
        for (int level = 0; level < 4; level++) {
            if (mlfqFront[level] < mlfqRear[level]) {
                pid = mlfq[level][mlfqFront[level]++];
                break;
            }
        }
    }
    
    if (pid != -1) {
        PCB *pcb = findPCBByPid(pid);
        if (pcb && strcmp(pcb->state, "Ready") == 0) {
            setState(pcb, "Running");
            
            char log_msg[100];
            sprintf(log_msg, "Clock Cycle %d: Executing process %d instruction %d", 
                  clockCycle, pid, pcb->programCounter);
            log_message(log_msg);
            
            int finished = executeInstruction(pcb, clockCycle);
            
            // Handle process post-execution
            if (strcmp(pcb->state, "Running") == 0) {
                setState(pcb, "Ready");
            }
        }
    } else {
        char log_msg[100];
        sprintf(log_msg, "Clock Cycle %d: No process to run", clockCycle);
        log_message(log_msg);
    }
    
    clockCycle++;
    
    // Update UI
    update_process_dashboard();
    update_memory_view();
    
    // Check if all processes are finished
    int all_finished = 1;
    for (int i = 0; i < processCount; i++) {
        if (strcmp(processList[i].pcb.state, "Finished") != 0) {
            all_finished = 0;
            break;
        }
    }
    
    if (all_finished && processCount > 0) {
        log_message("All processes finished execution");
        scheduler_running = FALSE;
        return FALSE; // stop timer
    }
    
    return TRUE; // continue timer
}

// Callback for start button
void on_start_clicked(GtkButton *button, gpointer user_data) {
    if (!scheduler_running) {
        GtkWidget *combo_box = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "combo_box"));
        GtkWidget *quantum_spin = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "quantum_spin"));
        
        schedulerType = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box)) + 1;
        rrQuantum = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(quantum_spin));
        
        sortProcessesByArrival();
        
        log_message("Starting scheduler...");
        scheduler_running = TRUE;
        
        // Run scheduler at 100ms intervals
        if (timeout_id == 0) {
            timeout_id = g_timeout_add(100, scheduler_step, NULL);
        }
    }
}

// Callback for pause button
void on_pause_clicked(GtkButton *button, gpointer user_data) {
    scheduler_running = FALSE;
    log_message("Scheduler paused");
}

// Callback for reset button
void on_reset_clicked(GtkButton *button, gpointer user_data) {
    scheduler_running = FALSE;
    if (timeout_id > 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
    
    // Reset simulator state
    initMemory();
    initMutexes();
    
    // Clear process list
    processCount = 0;
    globalPcbCount = 0;
    
    // Reset UI
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(g_process_treeview)));
    gtk_list_store_clear(store);
    
    GtkWidget *process_list = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "process_list"));
    if (process_list) {  // Add this NULL check
        GtkListBox *list_box = GTK_LIST_BOX(process_list);
        GList *children, *iter;
        
        children = gtk_container_get_children(GTK_CONTAINER(list_box));
        for (iter = children; iter != NULL; iter = g_list_next(iter))
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        g_list_free(children);
    }
    
    update_memory_view();
    
    log_message("System reset");
    clockCycle = 0;
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *main_vbox;
    GtkWidget *control_panel;
    GtkWidget *content_hbox;
    GtkWidget *left_panel;
    GtkWidget *center_panel;
    GtkWidget *right_panel;
    GtkWidget *log_panel;

    // Initialize backend
    initMemory();
    initMutexes();
    
    // Create main application window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Scheduler Simulator");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 700);

    // Main vertical container
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // First create the content_hbox
    content_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    // Create the left panel before control panel (since control panel needs it)
    left_panel = init_process_config_panel();
    
    // Then create control panel (which needs left_panel)
    control_panel = init_control_panel(left_panel);
    gtk_box_pack_start(GTK_BOX(main_vbox), control_panel, FALSE, FALSE, 5);
    
    // Now add the content box to main layout
    gtk_box_pack_start(GTK_BOX(main_vbox), content_hbox, TRUE, TRUE, 0);
    
    // Add left panel to content box
    gtk_box_pack_start(GTK_BOX(content_hbox), left_panel, FALSE, FALSE, 5);

    // Center: Process Dashboard
    center_panel = init_process_dashboard();
    gtk_box_pack_start(GTK_BOX(content_hbox), center_panel, TRUE, TRUE, 5);

    // Right: Memory Viewer
    right_panel = init_memory_viewer();
    gtk_box_pack_start(GTK_BOX(content_hbox), right_panel, FALSE, FALSE, 5);

    // Bottom: Log Panel
    log_panel = init_log_panel();
    gtk_box_pack_start(GTK_BOX(main_vbox), log_panel, FALSE, FALSE, 5);

    // Show the main window and all its children
    gtk_widget_show_all(window);
}

// Initialize the scheduler control panel
GtkWidget* init_control_panel(GtkWidget *left_panel) {
    GtkWidget *frame, *hbox, *label, *combo_box, *quantum_label, *quantum_spin;
    GtkWidget *button_box, *start_button, *pause_button, *reset_button;
    
    // Create frame with title
    frame = gtk_frame_new("Scheduler Control Panel");
    
    // Create horizontal box for controls
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    
    // Scheduler selection dropdown
    label = gtk_label_new("Scheduler Type:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    
    combo_box = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "FCFS");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "MLFQ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
    gtk_box_pack_start(GTK_BOX(hbox), combo_box, FALSE, FALSE, 5);
    
    // Quantum setting (initially hidden)
    quantum_label = gtk_label_new("Quantum (ms):");
    gtk_box_pack_start(GTK_BOX(hbox), quantum_label, FALSE, FALSE, 5);
    
    quantum_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(quantum_spin), 20);
    gtk_box_pack_start(GTK_BOX(hbox), quantum_spin, FALSE, FALSE, 5);
    
    // Control buttons
    button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end(GTK_BOX(hbox), button_box, FALSE, FALSE, 5);
    
    start_button = gtk_button_new_with_label("Start");
    pause_button = gtk_button_new_with_label("Pause");
    reset_button = gtk_button_new_with_label("Reset");
    
    gtk_box_pack_start(GTK_BOX(button_box), start_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), pause_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), reset_button, FALSE, FALSE, 5);

    // Connect signals
    g_signal_connect(combo_box, "changed", G_CALLBACK(on_scheduler_type_changed), quantum_label);
    g_object_set_data(G_OBJECT(quantum_label), "quantum_spin", quantum_spin);
    
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    g_object_set_data(G_OBJECT(start_button), "combo_box", combo_box);
    g_object_set_data(G_OBJECT(start_button), "quantum_spin", quantum_spin);
    
    g_signal_connect(pause_button, "clicked", G_CALLBACK(on_pause_clicked), NULL);
    g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_clicked), left_panel);

    GtkWidget *proc_list = GTK_WIDGET(g_object_get_data(G_OBJECT(left_panel), "process_list"));
    g_object_set_data(G_OBJECT(reset_button), "process_list", proc_list);
    
    // Initially hide quantum controls (only for RR)
    gtk_widget_set_visible(quantum_label, FALSE);
    gtk_widget_set_visible(quantum_spin, FALSE);
    
    return frame;
}

// Initialize the process configuration panel
GtkWidget* init_process_config_panel() {
    GtkWidget *frame, *vbox, *label, *file_entry, *hbox;
    GtkWidget *arrival_label, *arrival_spin, *add_button, *scrolled_window, *process_list;
    
    // Create frame with title
    frame = gtk_frame_new("Process Configuration");
    gtk_widget_set_size_request(frame, 250, -1);
    
    // Main vertical layout
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    
    // File path entry
    label = gtk_label_new("Process File:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    file_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), file_entry, FALSE, FALSE, 0);
    
    // Arrival time controls
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
    
    arrival_label = gtk_label_new("Arrival Time:");
    gtk_box_pack_start(GTK_BOX(hbox), arrival_label, FALSE, FALSE, 0);
    
    arrival_spin = gtk_spin_button_new_with_range(0, 1000, 1);
    gtk_box_pack_start(GTK_BOX(hbox), arrival_spin, TRUE, TRUE, 0);
    
    // Add process button
    add_button = gtk_button_new_with_label("Add Process");
    gtk_box_pack_start(GTK_BOX(vbox), add_button, FALSE, FALSE, 5);
    
    // Process list
    label = gtk_label_new("Added Processes:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
    
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, -1, 200);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    process_list = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), process_list);

    // Connect add button signal
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_process_clicked), NULL);
    g_object_set_data(G_OBJECT(add_button), "file_entry", file_entry);
    g_object_set_data(G_OBJECT(add_button), "arrival_spin", arrival_spin);
    g_object_set_data(G_OBJECT(add_button), "process_list", process_list);
    g_object_set_data(G_OBJECT(frame), "process_list", process_list);

    return frame;
}

// Initialize the process dashboard panel
GtkWidget* init_process_dashboard() {
    GtkWidget *frame, *scrolled_window, *treeview;
    GtkListStore *store;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    
    // Create frame with title
    frame = gtk_frame_new("Process Dashboard");
    
    // Create a scrolled window to contain the treeview
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
    
    // Create list store for process data
    // Columns: PID, State, Priority, PC, Memory Start, Memory End
    store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, 
                              G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    
    // Create treeview
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    // Column 1: Process ID
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    // Column 2: State
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("State", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    // Column 3: Priority
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    // Column 4: PC
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PC", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    // Column 5-6: Memory boundaries
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Mem Start", renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Mem End", renderer, "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    // Add treeview to scrolled window
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);

    // Store global reference
    g_process_treeview = treeview;
    
    return frame;
}

// Initialize the memory viewer panel
GtkWidget* init_memory_viewer() {
    GtkWidget *frame, *scrolled_window, *grid;
    GtkWidget *label;
    char buffer[16];
    int i, row, col;
    
    // Create frame with title
    frame = gtk_frame_new("Memory Viewer");
    gtk_widget_set_size_request(frame, 250, -1);
    
    // Create a scrolled window
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
    
    // Create grid for memory cells (4 columns x 15 rows = 60 cells)
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    
    // Add header row
    label = gtk_label_new("Addr");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    
    label = gtk_label_new("Value");
    gtk_grid_attach(GTK_GRID(grid), label, 1, 0, 1, 1);
    
    label = gtk_label_new("Addr");
    gtk_grid_attach(GTK_GRID(grid), label, 2, 0, 1, 1);
    
    label = gtk_label_new("Value");
    gtk_grid_attach(GTK_GRID(grid), label, 3, 0, 1, 1);
    
    // Create 60 memory cells
    for (i = 0; i < 60; i++) {
        row = (i / 2) + 1;  // +1 because of header row
        col = (i % 2) * 2;  // 0 or 2
        
        // Address label
        sprintf(buffer, "%02d:", i);
        label = gtk_label_new(buffer);
        gtk_widget_set_halign(label, GTK_ALIGN_END);
        gtk_grid_attach(GTK_GRID(grid), label, col, row, 1, 1);
        
        // Value label (initially empty)
        label = gtk_label_new("0");
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), label, col + 1, row, 1, 1);
    }
    
    gtk_container_add(GTK_CONTAINER(scrolled_window), grid);

    // Store global reference
    g_memory_grid = grid;
    
    return frame;
}

// Initialize the log panel
GtkWidget* init_log_panel() {
    GtkWidget *frame, *scrolled_window, *text_view;
    GtkTextBuffer *buffer;
    
    // Create frame with title
    frame = gtk_frame_new("System Log");
    
    // Create a scrolled window
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, -1, 150);
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
    
    // Create text view for logs
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
    
    // Set initial text
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, "System initialized. Ready for scheduling.", -1);
    
    // Add text view to scrolled window
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    // Store global reference
    g_log_textview = text_view;
    
    return frame;
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.scheduler", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}