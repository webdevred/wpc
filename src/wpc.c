#include <dirent.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

static gchar **list_images(gchar *source_directory, int *number_of_images) {
    gchar **filenamesArray = NULL;
    int filenameArraySize = 0;
    DIR *dir;
    struct dirent *file;

    dir = opendir(source_directory);
    if (dir) {
        while ((file = readdir(dir)) != NULL) {
            char *filename = file->d_name;

            if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
                continue;
            }

            filenamesArray = realloc(filenamesArray,
                                     (filenameArraySize + 1) * sizeof(gchar *));
            if (!filenamesArray) {
                closedir(dir);
            }

            filenamesArray[filenameArraySize] =
                malloc(strlen(source_directory) * sizeof(gchar *) +
                       strlen(filename) * sizeof(gchar *) + sizeof(gchar *));
            if (filenamesArray[filenameArraySize]) {
                gchar *image_path =
                    g_strconcat(source_directory, filename, NULL);
                strcpy(filenamesArray[filenameArraySize], image_path);
                filenameArraySize++;
            }
        }
        closedir(dir);

        filenamesArray =
            realloc(filenamesArray, (filenameArraySize + 1) * sizeof(gchar *));
        filenamesArray[filenameArraySize] = NULL;
    }

    *number_of_images = filenameArraySize;

    return filenamesArray;
}

gchar *log_resolution(int x, int y) {
    int expected_length = snprintf(NULL, 0, "%dx%d", x, y) + 1;
    gchar *output = malloc(expected_length * sizeof(gchar));
    sprintf(output, "%dx%d", x, y);
    return output;
}

static void show_image(GtkWidget *image, gchar *image_path,
                       int new_image_width) {
    gtk_image_set_from_file(GTK_IMAGE(image), image_path);

    GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
    if (pixbuf) {
        int original_width = gdk_pixbuf_get_width(pixbuf);
        int original_height = gdk_pixbuf_get_height(pixbuf);

        double scale = (double)new_image_width / (double)original_width;
        int new_height = (int)(original_height * scale);

        gchar *old_res = log_resolution(original_width, original_height);
        gchar *new_res = log_resolution(new_image_width, new_height);

        g_print("Image path: %-40s Original resolution: %-10s New resolution: "
                "%-10s\n",
                image_path, old_res, new_res);

        GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(
            pixbuf, new_image_width, new_height, GDK_INTERP_BILINEAR);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), scaled_pixbuf);

        g_object_unref(scaled_pixbuf);
    }
}

static void image_clicked(GtkWidget *widget, GdkEventButton *event,
                          gpointer user_data) {
    gchar *image_path = g_object_get_data(G_OBJECT(widget), "image_path");

    g_print("Clicked image %s\n", image_path);
}

void free_image_paths(gpointer data) {
    gchar **image_paths = (gchar **)data;
    if (image_paths) {
        for (int i = 0; image_paths[i] != NULL; i++) {
            g_free(image_paths[i]);
        }
        g_free(image_paths);
    }
}

void destroy_all_widgets(GtkWidget *container) {
    GList *children, *iter;

    // Get the list of children in the container
    children = gtk_container_get_children(GTK_CONTAINER(container));

    // Iterate over all children and destroy them
    for (iter = children; iter != NULL; iter = iter->next) {
        GtkWidget *child = GTK_WIDGET(iter->data);
        gtk_widget_destroy(child);
    }

    // Free the list of children (but don't destroy the widgets again)
    g_list_free(children);
}

static void on_window_realized(GtkWidget *window, GdkEvent *event,
                               gpointer user_data) {
    GtkApplication *app = GTK_APPLICATION(user_data);
    int image_width = 500;

    gint window_width = gtk_widget_get_allocated_width(window);

    int number_of_images;
    gchar *source_directory = "/mnt/HDD/backgrounds/";
    gchar **image_paths = list_images(source_directory, &number_of_images);

    destroy_all_widgets(window);

    GtkWidget *gridbox = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(gridbox), 10);
    gtk_grid_set_row_spacing(GTK_GRID(gridbox), 10);

    gtk_container_add(GTK_CONTAINER(window), gridbox);

    if (image_paths) {
        int columns = window_width / image_width;
        int current_column = 0;
        int current_row = 0;

        for (int i = 0; i < number_of_images; i++) {
            g_print("%d %d %d", current_row, current_column, window_width);
            GtkWidget *event_box = gtk_event_box_new();
            GtkWidget *image = gtk_image_new();
            gtk_container_add(GTK_CONTAINER(event_box), image);
            gtk_grid_attach(GTK_GRID(gridbox), event_box, current_column,
                            current_row, 1, 1);

            show_image(image, image_paths[i], image_width);

            g_object_set_data(G_OBJECT(event_box), "image_path",
                              (gpointer)image_paths[i]);
            g_signal_connect(event_box, "button_press_event",
                             G_CALLBACK(image_clicked), app);

            current_column += image_width;

            if (current_column >= columns * image_width) {
                current_column = 0;
                current_row += image_width;
            }
        }

        g_object_set_data_full(G_OBJECT(window), "image_paths", image_paths,
                               free_image_paths);
    } else {
        g_print("No images found in %s\n", source_directory);
    }

    gtk_widget_show_all(window);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
    g_signal_connect(window, "configure-event", G_CALLBACK(on_window_realized),
                     (gpointer)app);
    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    /* Monitor *monitors = list_monitors(); */
    app = gtk_application_new("org.webdevred.wpc", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
