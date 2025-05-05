// gui_process.c
#include <gtk/gtk.h>
#include "../gui/gui_process.h"
#include "../gui/gui_log.h"
#include "../include/process.h"

GtkWidget *process_view;
GtkListStore *process_store;

GtkWidget* gui_create_process_panel() {
    GtkWidget *frame = gtk_frame_new("Process List");
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled, 600, 200);

    process_store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    process_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(process_store));

    const char *titles[] = {"PID", "State", "Priority", "PC", "Mem Start", "Mem End"};
    for (int i = 0; i < 6; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(process_view), column);
    }

    gtk_container_add(GTK_CONTAINER(scrolled), process_view);
    gtk_container_add(GTK_CONTAINER(frame), scrolled);
    return frame;
}

void gui_process_add_row(PCB p) {
    gtk_list_store_insert_with_values(process_store, NULL, -1,
        0, p.pid,
        1, p.state,
        2, p.priority,
        3, p.programCounter,
        4, p.memLowerBound,
        5, p.memUpperBound,
        -1);
}

void gui_process_refresh_all() {
    gtk_list_store_clear(process_store);
    for (int i = 0; i < processCount; i++) {
        gui_process_add_row(processList[i].pcb);
    }
}
