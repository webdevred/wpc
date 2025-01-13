#include <dirent.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

static gchar **list_images(gchar *source_directory) {
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
                    g_strdup(g_strconcat(source_directory, filename, NULL));
                strcpy(filenamesArray[filenameArraySize], image_path);
                filenameArraySize++;
            }
        }
        closedir(dir);

        filenamesArray =
            realloc(filenamesArray, (filenameArraySize + 1) * sizeof(gchar *));
        filenamesArray[filenameArraySize] = NULL;
    }

    return filenamesArray;
}

static void show_image(GtkWidget *image, gchar *image_path) {
    gtk_image_set_from_file(GTK_IMAGE(image), image_path);

    GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
    if (pixbuf) {
        int original_width = gdk_pixbuf_get_width(pixbuf);
        int original_height = gdk_pixbuf_get_height(pixbuf);

        int new_width = 500;
        double scale = (double)new_width / (double)original_width;
        int new_height = (int)(original_height * scale);

        g_print("Image path: %-40s Original resolution: %dx%d New resolution: "
                "%dx%d\n",
                image_path, original_width, original_height, new_width,
                new_height);

        GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(
            pixbuf, new_width, new_height, GDK_INTERP_BILINEAR);

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

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_add(GTK_CONTAINER(window), hbox);

    gchar *source_directory = "/mnt/HDD/backgrounds/";
    gchar **image_paths = list_images(source_directory);

    if (image_paths) {
        // Store image paths in the window for cleanup
        g_object_set_data_full(G_OBJECT(window), "image_paths",
                               (gpointer)image_paths, free_image_paths);

        for (int i = 0; image_paths[i] != NULL; i++) {
            GtkWidget *event_box;
            GtkWidget *image = gtk_image_new();

            event_box = gtk_event_box_new();
            gtk_container_add(GTK_CONTAINER(event_box), image);
            gtk_box_pack_start(GTK_BOX(hbox), event_box, TRUE, TRUE, 0);

            show_image(image, image_paths[i]);

            g_object_set_data(G_OBJECT(event_box), "image_path",
                              (gpointer)image_paths[i]);

            g_signal_connect_data(G_OBJECT(event_box), "button_press_event",
                                  G_CALLBACK(image_clicked), (gpointer)app,
                                  NULL, 0);
        }
    } else {
        g_print("No images found in %s\n", source_directory);
    }

    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.webdevred.wpc", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
