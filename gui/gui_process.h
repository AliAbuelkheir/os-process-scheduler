#ifndef GUI_MEMORY_H
#define GUI_MEMORY_H

#include <gtk/gtk.h>

// Returns the widget containing the memory visualization
GtkWidget* init_memory_panel(void);

// Updates the memory grid from backend
void update_memory_view(void);

#endif // GUI_MEMORY_H
