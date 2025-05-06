#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/memory.h"
#include "../include/process.h"
#include "../include/scheduler.h"
#include "../include/mutex.h"
#include "../include/fileio.h"
#include "../include/interpreter.h"

void initMemory();
void initMutexes();

GtkWidget* init_control_panel(GtkWidget *proc_config_panel);
GtkWidget* init_process_config_panel();
GtkWidget* init_process_dashboard();
GtkWidget* init_memory_viewer();
GtkWidget* init_log_panel();
GtkWidget* init_overview_panel();
void update_overview_panel(GtkWidget *overview_panel);
void log_message(const char *message);

GtkWidget *g_process_treeview;
GtkWidget *g_log_textview;
GtkWidget *g_memory_grid;
gboolean scheduler_running = FALSE;
guint timeout_id = 0;

gboolean check_processes_loaded() {
    if (processCount <= 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "No processes loaded. Please add processes before starting the scheduler.");
        
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        
        log_message("Warning: Attempted to start scheduler with no processes");
        return FALSE;
    }
    return TRUE;
}

void update_process_dashboard() {
    GtkListStore *store;
    GtkTreeIter iter;
    
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(g_process_treeview)));
    gtk_list_store_clear(store);
    
    for (int i = 0; i < processCount; i++) {
        ProcessInfo *info = &processList[i];
        PCB *pcb = &info->pcb;
        
        if (info->loaded) {
            const char *state = getState(pcb);
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                              0, pcb->pid,
                              1, state ? state : "Unknown",
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
    char buffer[256];

    for (int i = 0; i < MEMORY_SIZE; i++) {
        int col = i / 30; // 30 rows per column
        int row = i % 30;

        GtkWidget *child = gtk_grid_get_child_at(GTK_GRID(g_memory_grid), col * 2 + 1, row);
        if (child && memory[i].key[0] != '\0') {
            sprintf(buffer, "%.30s: %.30s", memory[i].key, memory[i].value);
            gtk_label_set_text(GTK_LABEL(child), buffer);
        } else if (child) {
            gtk_label_set_text(GTK_LABEL(child), "(empty)");
        }
    }
}

void on_scheduler_type_changed(GtkComboBox *combo_box, gpointer user_data) {
    GtkWidget *quantum_label = GTK_WIDGET(user_data);
    GtkWidget *quantum_spin = GTK_WIDGET(g_object_get_data(G_OBJECT(quantum_label), "quantum_spin"));
    GtkWidget *overview_panel = GTK_WIDGET(g_object_get_data(G_OBJECT(combo_box), "overview_panel"));

    int active = gtk_combo_box_get_active(combo_box);

    if (active == 2) { // Round Robin
        gtk_widget_set_visible(quantum_label, TRUE);
        gtk_widget_set_visible(quantum_spin, TRUE);
    } else {
        gtk_widget_set_visible(quantum_label, FALSE);
        gtk_widget_set_visible(quantum_spin, FALSE);
    }

    // Update scheduler type in the backend
    schedulerType = active; // 0 = None, 1 = FCFS, 2 = RR, 3 = MLFQ

    // Update the System Overview
    update_overview_panel(overview_panel);
}

GtkWidget* init_queue_section() {
    GtkWidget *frame, *vbox, *label;

    // Create frame with title
    frame = gtk_frame_new("Queue Section");
    gtk_widget_set_size_request(frame, 300, 150);

    // Create vertical box layout
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    // Ready Queue
    label = gtk_label_new("Ready Queue: ");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
    g_object_set_data(G_OBJECT(frame), "ready_queue_label", label);

    // Running Process
    label = gtk_label_new("Running Process: None");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
    g_object_set_data(G_OBJECT(frame), "running_process_label", label);

    // Blocking Queue
    label = gtk_label_new("Blocking Queue: ");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
    g_object_set_data(G_OBJECT(frame), "blocking_queue_label", label);

    return frame;
}

GtkWidget* init_resource_management_panel() {
    GtkWidget *frame, *vbox, *label;

    // Create frame with title
    frame = gtk_frame_new("Resource Management Panel");
    gtk_widget_set_size_request(frame, 300, 150);

    // Create vertical box layout
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    // Mutex Status
    label = gtk_label_new("Mutex Status:\nuserInput: -1\nuserOutput: -1\nfile: -1");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
    g_object_set_data(G_OBJECT(frame), "mutex_status_label", label);

    // Blocked Queues
    label = gtk_label_new("Blocked Queues:\nuserInput: \nuserOutput: \nfile: ");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
    g_object_set_data(G_OBJECT(frame), "blocked_queue_label", label);

    return frame;
}

void update_queue_section(GtkWidget *queue_panel) {
    GtkWidget *ready_queue_label = GTK_WIDGET(g_object_get_data(G_OBJECT(queue_panel), "ready_queue_label"));
    GtkWidget *running_process_label = GTK_WIDGET(g_object_get_data(G_OBJECT(queue_panel), "running_process_label"));
    GtkWidget *blocking_queue_label = GTK_WIDGET(g_object_get_data(G_OBJECT(queue_panel), "blocking_queue_label"));

    char buffer[512];

    // Update Ready Queue
    sprintf(buffer, "Ready Queue:\n");
    for (int i = 0; i < processCount; i++) {
        PCB *pcb = &processList[i].pcb;
        const char *state = getState(pcb);
        if (state && strcmp(state, "Ready") == 0) {
            char temp[128];
            const char* instruction = getCurrentInstruction(pcb);
            sprintf(temp, "PID %d | Next Instruction: %s\n", 
                    pcb->pid, instruction ? instruction : "No instruction");
            strcat(buffer, temp);
        }
    }
    gtk_label_set_text(GTK_LABEL(ready_queue_label), buffer);

    // Update Running Process
    sprintf(buffer, "Running Process: None");
    for (int i = 0; i < processCount; i++) {
        PCB *pcb = &processList[i].pcb;
        const char *state = getState(pcb);
        if (state && strcmp(state, "Running") == 0) {
            const char* instruction = getCurrentInstruction(pcb);
            sprintf(buffer, "Running Process:\nPID %d | Executing: %s", 
                    pcb->pid, instruction ? instruction : "No instruction");
            break;
        }
    }
    gtk_label_set_text(GTK_LABEL(running_process_label), buffer);

    // Update Blocking Queue
    sprintf(buffer, "Blocking Queue:\n");
    for (int i = 0; i < processCount; i++) {
        PCB *pcb = &processList[i].pcb;
        const char *state = getState(pcb);
        if (state && strcmp(state, "Blocked") == 0) {
            char temp[128];
            sprintf(temp, "PID %d\n", pcb->pid);
            strcat(buffer, temp);
        }
    }
    gtk_label_set_text(GTK_LABEL(blocking_queue_label), buffer);
}

void update_resource_management_panel(GtkWidget *resource_panel) {
    char buffer[1024];
    GtkWidget *mutex_status_label = GTK_WIDGET(g_object_get_data(G_OBJECT(resource_panel), "mutex_status_label"));
    
    // Update Mutex Status with detailed information
    sprintf(buffer, "Mutex Status:\n");
    
    // userInput mutex
    strcat(buffer, "userInput: ");
    if (userInput != -1) {
        char temp[128];
        sprintf(temp, "Held by P%d", userInput);
        strcat(buffer, temp);
        if (inputCount > 0) {
            strcat(buffer, "\n  Waiting processes: ");
            for (int i = 0; i < inputCount; i++) {
                sprintf(temp, "P%d(priority:%d) ", 
                        userInputQueue[i], 
                        getPriority(userInputQueue[i]));
                strcat(buffer, temp);
            }
        }
    } else {
        strcat(buffer, "Available");
    }
    
    // userOutput mutex
    strcat(buffer, "\n\nuserOutput: ");
    if (userOutput != -1) {
        char temp[128];
        sprintf(temp, "Held by P%d", userOutput);
        strcat(buffer, temp);
        if (outputCount > 0) {
            strcat(buffer, "\n  Waiting processes: ");
            for (int i = 0; i < outputCount; i++) {
                sprintf(temp, "P%d(priority:%d) ", 
                        userOutputQueue[i], 
                        getPriority(userOutputQueue[i]));
                strcat(buffer, temp);
            }
        }
    } else {
        strcat(buffer, "Available");
    }
    
    // file mutex
    strcat(buffer, "\n\nfile: ");
    if (file != -1) {
        char temp[128];
        sprintf(temp, "Held by P%d", file);
        strcat(buffer, temp);
        if (fileCount > 0) {
            strcat(buffer, "\n  Waiting processes: ");
            for (int i = 0; i < fileCount; i++) {
                sprintf(temp, "P%d(priority:%d) ", 
                        fileQueue[i], 
                        getPriority(fileQueue[i]));
                strcat(buffer, temp);
            }
        }
    } else {
        strcat(buffer, "Available");
    }
    
    gtk_label_set_text(GTK_LABEL(mutex_status_label), buffer);
}

void on_add_process_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *file_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "file_entry"));
    GtkWidget *arrival_spin = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "arrival_spin"));
    
    const char *filename = gtk_entry_get_text(GTK_ENTRY(file_entry));
    int arrival_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(arrival_spin));
    
    if (strlen(filename) > 0) {
        addProcess(filename, arrival_time);
        
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

gboolean scheduler_step(gpointer user_data) {
    GtkWidget *window = GTK_WIDGET(user_data);
    if (!GTK_IS_WIDGET(window)) {
        g_warning("Invalid window reference in scheduler");
        return FALSE;
    }

    if (!scheduler_running) {
        timeout_id = 0;
        return FALSE;
    }

    // Check if all processes are finished
    // if (getFinishedProcessCount() == getTotalProcessCount()) {
    //     log_message("All processes finished execution");
    //     scheduler_running = FALSE;
    //     timeout_id = 0;
    //     return FALSE;
    // }

    // Advance the scheduler
    advanceScheduler();

    // Update all panels
    GtkWidget *overview_panel = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "overview_panel"));
    GtkWidget *queue_panel = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "queue_panel"));
    GtkWidget *resource_panel = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "resource_panel"));

    if (overview_panel) update_overview_panel(overview_panel);
    if (queue_panel) update_queue_section(queue_panel);
    if (resource_panel) update_resource_management_panel(resource_panel);

    update_process_dashboard();
    update_memory_view();

    // Force immediate update
    while (gtk_events_pending())
        gtk_main_iteration();

    return TRUE;
}

void on_start_clicked(GtkButton *button, gpointer user_data) {
    if (!check_processes_loaded()) return;

    if (!scheduler_running) {
        GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
        GtkWidget *combo_box = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "combo_box"));
        GtkWidget *quantum_spin = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "quantum_spin"));

        schedulerType = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box)) + 1;
        rrQuantum = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(quantum_spin));

        // Initialize and run first cycle only
        initScheduler(schedulerType, rrQuantum);
        sortProcessesByArrival();
        scheduler_running = TRUE;
        scheduler_step(window);
        scheduler_running = FALSE;
    }
}

void on_pause_clicked(GtkButton *button, gpointer user_data) {
    scheduler_running = FALSE;
    log_message("Scheduler paused");
}

void on_reset_clicked(GtkButton *button, gpointer user_data) {
    scheduler_running = FALSE;
    if (timeout_id > 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }

    resetScheduler();

    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(g_process_treeview)));
    gtk_list_store_clear(store);

    GtkWidget *process_list = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "process_list"));
    if (process_list) {
        GtkListBox *list_box = GTK_LIST_BOX(process_list);
        GList *children, *iter;

        children = gtk_container_get_children(GTK_CONTAINER(list_box));
        for (iter = children; iter != NULL; iter = g_list_next(iter))
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        g_list_free(children);
    }

    update_memory_view();

    // Clear system log
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g_log_textview));
    gtk_text_buffer_set_text(buffer, "", -1);

    log_message("System reset");
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *main_vbox;
    GtkWidget *control_panel;
    GtkWidget *content_hbox;
    GtkWidget *top_hbox; // New horizontal box for System Overview, Queue Section, and Resource Management Panel
    GtkWidget *left_panel;
    GtkWidget *center_panel;
    GtkWidget *right_panel;
    GtkWidget *log_panel;
    GtkWidget *overview_panel;
    GtkWidget *queue_panel;
    GtkWidget *resource_panel;

    // Initialize backend
    initMemory();
    initMutexes();
    
    // Create main application window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Scheduler Simulator");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 700);

    gtk_window_fullscreen(GTK_WINDOW(window));

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

    // Create a horizontal box for System Overview, Queue Section, and Resource Management Panel
    top_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), top_hbox, FALSE, FALSE, 5);

    // Add the Overview Section
    overview_panel = init_overview_panel();
    gtk_box_pack_start(GTK_BOX(top_hbox), overview_panel, TRUE, TRUE, 5);

    // Add the Queue Section
    queue_panel = init_queue_section();
    gtk_box_pack_start(GTK_BOX(top_hbox), queue_panel, TRUE, TRUE, 5);

    // Add the Resource Management Panel
    resource_panel = init_resource_management_panel();
    gtk_box_pack_start(GTK_BOX(top_hbox), resource_panel, TRUE, TRUE, 5);
    
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

    // Store references for updates
    g_object_set_data(G_OBJECT(window), "overview_panel", overview_panel);
    g_object_set_data(G_OBJECT(window), "queue_panel", queue_panel);
    g_object_set_data(G_OBJECT(window), "resource_panel", resource_panel);
    // Modify the size requests for top panels
    gtk_widget_set_size_request(overview_panel, 300, 100); // Make System Overview shorter
    gtk_widget_set_size_request(queue_panel, 300, 100); // Make Queue Section shorter
    gtk_widget_set_size_request(resource_panel, 300, 100); // Make Resource Panel shorter

    // Make the log panel bigger
    gtk_widget_set_size_request(log_panel, -1, 300); // Increase height of log panel
}

void on_step_execution_clicked(GtkButton *button, gpointer user_data) {
    if (!check_processes_loaded()) return;

    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    scheduler_running = TRUE;
    scheduler_step(window);
    scheduler_running = FALSE;
}

void on_auto_execution_clicked(GtkButton *button, gpointer user_data) {
    if (!check_processes_loaded()) {
        return;
    }

    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    scheduler_running = TRUE;
    if (timeout_id == 0) {
        timeout_id = g_timeout_add(500, scheduler_step, window); // 500ms delay between steps
    }
}

GtkWidget* init_control_panel(GtkWidget *left_panel) {
    GtkWidget *frame, *hbox, *label, *combo_box, *quantum_label, *quantum_spin;
    GtkWidget *button_box, *start_button, *pause_button, *reset_button;
    GtkWidget *step_button, *auto_button; // Add Step and Auto buttons
    
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
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "Choose your scheduler");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "FCFS");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "MLFQ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0); // Default to "Choose your scheduler"
    gtk_box_pack_start(GTK_BOX(hbox), combo_box, FALSE, FALSE, 5);
    
    // Quantum setting (initially hidden)
    quantum_label = gtk_label_new("Quantum (ms):");
    gtk_box_pack_start(GTK_BOX(hbox), quantum_label, FALSE, FALSE, 5);
    
    quantum_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(quantum_spin), 1);
    gtk_box_pack_start(GTK_BOX(hbox), quantum_spin, FALSE, FALSE, 5);
    
    // Initially hide quantum controls
    gtk_widget_set_visible(quantum_label, FALSE);
    gtk_widget_set_visible(quantum_spin, FALSE);
    
    // Control buttons
    button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end(GTK_BOX(hbox), button_box, FALSE, FALSE, 5);
    
    start_button = gtk_button_new_with_label("Start");
    pause_button = gtk_button_new_with_label("Pause");
    reset_button = gtk_button_new_with_label("Reset");
    step_button = gtk_button_new_with_label("Step"); // Step Execution Button
    auto_button = gtk_button_new_with_label("Auto"); // Auto Execution Button
    
    gtk_box_pack_start(GTK_BOX(button_box), start_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), pause_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), reset_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), step_button, FALSE, FALSE, 5); // Add Step Button
    gtk_box_pack_start(GTK_BOX(button_box), auto_button, FALSE, FALSE, 5); // Add Auto Button

     // Store window reference
     GtkWidget *window = gtk_widget_get_toplevel(left_panel);
     g_object_set_data(G_OBJECT(start_button), "window", window);
     g_object_set_data(G_OBJECT(step_button), "window", window);
     g_object_set_data(G_OBJECT(auto_button), "window", window);

    // Connect signals
    g_signal_connect(combo_box, "changed", G_CALLBACK(on_scheduler_type_changed), quantum_label);
    g_object_set_data(G_OBJECT(quantum_label), "quantum_spin", quantum_spin);
    
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    g_object_set_data(G_OBJECT(start_button), "combo_box", combo_box);
    g_object_set_data(G_OBJECT(start_button), "quantum_spin", quantum_spin);
    
    g_signal_connect(pause_button, "clicked", G_CALLBACK(on_pause_clicked), NULL);
    g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_clicked), left_panel);
    g_signal_connect(step_button, "clicked", G_CALLBACK(on_step_execution_clicked), NULL); // Connect Step Button
    g_signal_connect(auto_button, "clicked", G_CALLBACK(on_auto_execution_clicked), NULL); // Connect Auto Button

    GtkWidget *proc_list = GTK_WIDGET(g_object_get_data(G_OBJECT(left_panel), "process_list"));
    g_object_set_data(G_OBJECT(reset_button), "process_list", proc_list);
    
    return frame;
}

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
    label = gtk_label_new("Process File (e.g., programs/Program_1.txt):");
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

GtkWidget* init_memory_viewer() {
    GtkWidget *frame, *scrolled_window, *grid;
    GtkWidget *label;
    char buffer[16];
    int rows = 30; // Number of rows per column

    frame = gtk_frame_new("Memory Viewer");
    gtk_widget_set_size_request(frame, 500, 400);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);

    // Create vertical layout
    for (int i = 0; i < MEMORY_SIZE; i++) {
        int col = i / rows;    // Column index
        int row = i % rows;    // Row index

        // Address label
        sprintf(buffer, "%03d:", i);
        label = gtk_label_new(buffer);
        gtk_widget_set_halign(label, GTK_ALIGN_END);
        gtk_grid_attach(GTK_GRID(grid), label, col * 2, row, 1, 1);

        // Value label
        label = gtk_label_new("(empty)");
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), label, col * 2 + 1, row, 1, 1);
    }

    gtk_container_add(GTK_CONTAINER(scrolled_window), grid);
    g_memory_grid = grid;
    return frame;
}

GtkWidget* init_log_panel() {
    GtkWidget *frame, *scrolled_window, *textview;
    
    frame = gtk_frame_new("System Log");
    
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
    textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), textview);
    
    g_log_textview = textview;
    
    return frame;
}

void log_message(const char *message) {
    GtkTextBuffer *buffer;
    GtkTextIter end;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g_log_textview));
    gtk_text_buffer_get_end_iter(buffer, &end);

    gtk_text_buffer_insert(buffer, &end, message, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);

    while (gtk_events_pending())
        gtk_main_iteration();
}

GtkWidget* init_overview_panel() {
    GtkWidget *frame, *hbox, *label;
    char buffer[64];

    frame = gtk_frame_new("System Overview");
    gtk_widget_set_size_request(frame, 300, 100);

    // Create horizontal box layout
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    // Total Processes
    GtkWidget *proc_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), proc_box, TRUE, TRUE, 0);
    
    label = gtk_label_new("Total Processes:");
    gtk_box_pack_start(GTK_BOX(proc_box), label, FALSE, FALSE, 0);
    
    sprintf(buffer, "%d", processCount);
    GtkWidget *process_count_label = gtk_label_new(buffer);
    gtk_box_pack_start(GTK_BOX(proc_box), process_count_label, FALSE, FALSE, 0);

    // Clock Cycle
    GtkWidget *clock_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), clock_box, TRUE, TRUE, 0);
    
    label = gtk_label_new("Clock Cycle:");
    gtk_box_pack_start(GTK_BOX(clock_box), label, FALSE, FALSE, 0);
    
    sprintf(buffer, "%d", clockCycle);
    GtkWidget *clock_cycle_label = gtk_label_new(buffer);
    gtk_box_pack_start(GTK_BOX(clock_box), clock_cycle_label, FALSE, FALSE, 0);

    // Scheduler
    GtkWidget *sched_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), sched_box, TRUE, TRUE, 0);
    
    label = gtk_label_new("Scheduler:");
    gtk_box_pack_start(GTK_BOX(sched_box), label, FALSE, FALSE, 0);
    
    const char *scheduler_name = "None";
    GtkWidget *scheduler_label = gtk_label_new(scheduler_name);
    gtk_box_pack_start(GTK_BOX(sched_box), scheduler_label, FALSE, FALSE, 0);

    // Store references for updates
    g_object_set_data(G_OBJECT(frame), "process_count_label", process_count_label);
    g_object_set_data(G_OBJECT(frame), "clock_cycle_label", clock_cycle_label);
    g_object_set_data(G_OBJECT(frame), "scheduler_label", scheduler_label);

    return frame;
}

void update_overview_panel(GtkWidget *overview_panel) {
    if (!overview_panel) {
        g_warning("overview_panel is NULL");
        return;
    }

    char buffer[64];
    GtkWidget *process_count_label = GTK_WIDGET(g_object_get_data(G_OBJECT(overview_panel), "process_count_label"));
    GtkWidget *clock_cycle_label = GTK_WIDGET(g_object_get_data(G_OBJECT(overview_panel), "clock_cycle_label"));
    GtkWidget *scheduler_label = GTK_WIDGET(g_object_get_data(G_OBJECT(overview_panel), "scheduler_label"));

    if (!process_count_label || !clock_cycle_label || !scheduler_label) {
        g_warning("One or more labels are NULL in overview panel");
        return;
    }

    sprintf(buffer, "%d", processCount);
    gtk_label_set_text(GTK_LABEL(process_count_label), buffer);

    sprintf(buffer, "%d", clockCycle);
    gtk_label_set_text(GTK_LABEL(clock_cycle_label), buffer);

    const char *scheduler_name = "None";
    if (schedulerType == 1) scheduler_name = "FCFS";
    else if (schedulerType == 2) scheduler_name = "Round Robin";
    else if (schedulerType == 3) scheduler_name = "MLFQ";
    
    gtk_label_set_text(GTK_LABEL(scheduler_label), scheduler_name);
}

void handle_user_input(PCB *pcb, const char *var) {
    GtkWidget *dialog, *content_area, *entry, *label;
    char prompt[256];
    
    // Create detailed log message
    sprintf(prompt, "[Process %d] Waiting for user input: Please enter value for variable '%s'", pcb->pid, var);
    log_message(prompt);
    
    dialog = gtk_dialog_new_with_buttons("Input Required",
                                       NULL,
                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       "_OK",
                                       GTK_RESPONSE_ACCEPT,
                                       NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    // Add prompt label
    label = gtk_label_new(prompt);
    gtk_container_add(GTK_CONTAINER(content_area), label);
    
    entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        const char *input = gtk_entry_get_text(GTK_ENTRY(entry));
        char key[30];
        sprintf(key, "P%d_%s", pcb->pid, var);
        
        // Log the input received
        char input_msg[256];
        sprintf(input_msg, "[Process %d] Received input for variable '%s': %s", pcb->pid, var, input);
        log_message(input_msg);
        
        setMemory(key, input);
    }
    gtk_widget_destroy(dialog);
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