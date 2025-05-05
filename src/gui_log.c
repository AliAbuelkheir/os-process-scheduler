// gui_log.c
#include <gtk/gtk.h>
#include "../gui/gui_log.h"

GtkWidget *log_view;
GtkTextBuffer *log_buffer;

GtkWidget* gui_create_log_panel() {
    GtkWidget *frame = gtk_frame_new("Execution Log");
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled, 300, 200);

    log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(log_view), FALSE);

    log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
    gtk_container_add(GTK_CONTAINER(scrolled), log_view);
    gtk_container_add(GTK_CONTAINER(frame), scrolled);

    return frame;
}

void gui_log_append(const char *message) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(log_buffer, &end);
    gtk_text_buffer_insert(log_buffer, &end, message, -1);
    gtk_text_buffer_insert(log_buffer, &end, "\n", -1);

    GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(gtk_widget_get_parent(log_view)));
    gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment));
}
