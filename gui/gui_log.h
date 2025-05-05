// gui_log.h
#ifndef GUI_LOG_H
#define GUI_LOG_H

#include <gtk/gtk.h>

GtkWidget* gui_create_log_panel();
void gui_log_append(const char *message);

#endif // GUI_LOG_H
