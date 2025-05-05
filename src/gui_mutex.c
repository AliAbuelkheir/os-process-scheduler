// gui_mutex.c
#include <gtk/gtk.h>
#include "../gui/gui_mutex.h"
#include "../include/mutex.h"

GtkWidget *mutex_labels[3];
GtkWidget *blocked_lists[3];
const char *mutex_names[] = {"userInput", "userOutput", "file"};

GtkWidget* gui_create_mutex_panel() {
    GtkWidget *frame = gtk_frame_new("Mutex Status");
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    for (int i = 0; i < 3; i++) {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        char label_text[64];
        snprintf(label_text, sizeof(label_text), "%s - Held by: None", mutex_names[i]);

        mutex_labels[i] = gtk_label_new(label_text);
        blocked_lists[i] = gtk_label_new("Blocked Queue: (empty)");

        gtk_box_pack_start(GTK_BOX(box), mutex_labels[i], FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(box), blocked_lists[i], FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 5);
    }

    return frame;
}

void gui_mutex_refresh() {
    const int *holders[] = { &userInput, &userOutput, &file };
    const int *counts[] = { &inputCount, &outputCount, &fileCount };
    const int *queues[] = { userInputQueue, userOutputQueue, fileQueue };

    for (int i = 0; i < 3; i++) {
        char label_buf[64];
        if (*holders[i] == -1)
            snprintf(label_buf, sizeof(label_buf), "%s - Held by: None", mutex_names[i]);
        else
            snprintf(label_buf, sizeof(label_buf), "%s - Held by: PID %d", mutex_names[i], *holders[i]);

        gtk_label_set_text(GTK_LABEL(mutex_labels[i]), label_buf);

        // Blocked queue
        char queue_buf[128] = "Blocked Queue: ";
        if (*counts[i] == 0) {
            strcat(queue_buf, "(empty)");
        } else {
            for (int j = 0; j < *counts[i]; j++) {
                char pid_buf[8];
                snprintf(pid_buf, sizeof(pid_buf), "%d ", queues[i][j]);
                strcat(queue_buf, pid_buf);
            }
        }
        gtk_label_set_text(GTK_LABEL(blocked_lists[i]), queue_buf);
    }
}


