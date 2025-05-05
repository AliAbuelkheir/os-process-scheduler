// gui_process.h
#ifndef GUI_PROCESS_H
#define GUI_PROCESS_H

#include <gtk/gtk.h>
#include "../include/process.h"

GtkWidget* gui_create_process_panel();
void gui_process_add_row(PCB p);
void gui_process_refresh_all();

#endif // GUI_PROCESS_H
