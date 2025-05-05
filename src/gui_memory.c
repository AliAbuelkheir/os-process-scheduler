// gui_memory.c
#include <gtk/gtk.h>
#include "../gui/gui_memory.h"
#include "../include/memory.h"

GtkWidget *memory_grid;
GtkWidget *memory_labels[60];

GtkWidget* gui_create_memory_panel() {
    GtkWidget *frame = gtk_frame_new("Memory Viewer (60 words)");
    memory_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(memory_grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(memory_grid), 2);

    for (int i = 0; i < 60; i++) {
        memory_labels[i] = gtk_label_new("-");
        gtk_widget_set_size_request(memory_labels[i], 40, 20);
        gtk_grid_attach(GTK_GRID(memory_grid), memory_labels[i], i % 10, i / 10, 1, 1);
    }

    gtk_container_add(GTK_CONTAINER(frame), memory_grid);
    return frame;
}

// void gui_memory_refresh() {
//     for (int i = 0; i < MEMORY_SIZE; i++) {
//         char buf[16];
//         int pid = -1;

//         if (memory[i].key[0] == '\0') {
//             snprintf(buf, sizeof(buf), "-");
//         } else if (memory[i].key[0] == 'P') {
//             sscanf(memory[i].key, "P%d", &pid);
//             snprintf(buf, sizeof(buf), "%d", pid);
//         } else {
//             snprintf(buf, sizeof(buf), "?");
//         }

//         gtk_label_set_text(GTK_LABEL(memory_labels[i]), buf);

//         GdkRGBA color;
//         if (pid == -1)
//             gdk_rgba_parse(&color, "#eeeeee");
//         else
//             gdk_rgba_parse(&color, (pid % 2 == 0) ? "#a5d6a7" : "#90caf9");

//         GtkStyleContext *context = gtk_widget_get_style_context(memory_labels[i]);
//         gtk_style_context_add_class(context, "memory-cell");
//         gtk_widget_override_background_color(memory_labels[i], GTK_STATE_FLAG_NORMAL, &color);
//     }
// }

void gui_memory_refresh() {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        char buf[16];
        int pid = -1;

        if (memory[i].key[0] == '\0') {
            snprintf(buf, sizeof(buf), "-");
        } else if (memory[i].key[0] == 'P') {
            sscanf(memory[i].key, "P%d", &pid);
            snprintf(buf, sizeof(buf), "%d", pid);
        } else {
            snprintf(buf, sizeof(buf), "?");
        }

        gtk_label_set_text(GTK_LABEL(memory_labels[i]), buf);

        // Create a unique CSS class per label
        GtkStyleContext *context = gtk_widget_get_style_context(memory_labels[i]);

        // Remove old class if needed (optional if you update dynamically)
        GList *classes = gtk_style_context_list_classes(context);
        for (GList *l = classes; l != NULL; l = l->next)
            gtk_style_context_remove_class(context, (const char*)l->data);
        g_list_free_full(classes, g_free);

        char class_name[32];
        snprintf(class_name, sizeof(class_name), "cell-%d", i);
        gtk_style_context_add_class(context, class_name);

        // Build CSS for this cell
        const char *color = (pid == -1) ? "#eeeeee" : (pid % 2 == 0 ? "#a5d6a7" : "#90caf9");
        char css_rule[128];
        snprintf(css_rule, sizeof(css_rule), ".%s { background-color: %s; }", class_name, color);

        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, css_rule, -1, NULL);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
        g_object_unref(provider);
    }
}
