#include <glib.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "fs.h"
#include "gui.h"
#include "monitors.h"
#include "resolution_scaling.h"

static gchar *log_resolution(int x, int y) {
    char *format = "%d x %d";
    int expected_length = snprintf(NULL, 0, format, x, y) + 1;
    gchar *output = malloc(expected_length * sizeof(gchar));
    sprintf(output, format, x, y);
    return output;
}
static void show_image(GtkWidget *image, char *image_path,
                       int new_image_width) {
    gchar *g_image_path = g_locale_to_utf8(image_path, -1, NULL, NULL, NULL);
    gtk_image_set_from_file(GTK_IMAGE(image), g_image_path);

    GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
    if (pixbuf) {
        int original_width = gdk_pixbuf_get_width(pixbuf);
        int original_height = gdk_pixbuf_get_height(pixbuf);

        int new_height =
            scale_height(original_width, original_height, new_image_width);

        gchar *old_res = log_resolution(original_width, original_height);
        gchar *new_res = log_resolution(new_image_width, new_height);

        g_print("Image path: %-40s Original resolution: %-10s New resolution: "
                "%-10s\n",
                image_path, old_res, new_res);

        GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(
            pixbuf, new_image_width, new_height, GDK_INTERP_BILINEAR);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), scaled_pixbuf);

        gtk_widget_set_size_request(image, new_image_width, new_height);
        g_object_unref(scaled_pixbuf);
    }
}

static char *get_flow_child_image_path(GtkFlowBoxChild *flow_child) {
    GtkWidget *image = gtk_bin_get_child(GTK_BIN(flow_child));

    return g_object_get_data(G_OBJECT(image), "image_path");
}

static void image_selected(GtkFlowBox *flowbox, gpointer user_data) {
    GtkApplication *app = GTK_APPLICATION(user_data);
    Monitor *monitor = g_object_get_data(G_OBJECT(app), "selected_monitor");

    GList *flowbox_children = gtk_flow_box_get_selected_children(flowbox);
    GtkFlowBoxChild *selected_children = flowbox_children->data;

    char *image_path = get_flow_child_image_path(selected_children);

    g_print("Clicked image %s Selected monitor: %dx%d\n", image_path,
            monitor->width, monitor->height);
}

static void free_image_paths(gpointer data) {
    char **image_paths = (char **)data;
    if (image_paths) {
        for (int i = 0; image_paths[i] != NULL; i++) {
            free(image_paths[i]);
        }
        free(image_paths);
    }
}

static void free_monitors(gpointer data) {
    Monitor *monitors = (Monitor *)data;
    free(monitors);
}

static void destroy_all_widgets(GtkWidget *container) {
    GList *children, *iter;

    children = gtk_container_get_children(GTK_CONTAINER(container));

    for (iter = children; iter != NULL; iter = iter->next) {
        GtkWidget *child = GTK_WIDGET(iter->data);
        gtk_widget_destroy(child);
    }

    g_list_free(children);
}

static gint sort_flow_images(GtkFlowBoxChild *child1, GtkFlowBoxChild *child2,
                             gpointer user_data) {
    (void)user_data;
    char *image_path1 = get_flow_child_image_path(child1);
    char *image_path2 = get_flow_child_image_path(child2);

    return strcmp(image_path1, image_path2);
}

static void display_images(GtkButton *button, GtkApplication *app) {
    int image_width = 500;
    int number_of_images;

    char *source_directory = "/mnt/HDD/backgrounds/";
    char **image_paths = list_images(source_directory, &number_of_images);

    Monitor *monitor = g_object_get_data(G_OBJECT(button), "monitor");

    GtkTextBuffer *buffer =
        g_object_get_data(G_OBJECT(app), "status_selected_monitor");
    gchar *selected_monitor = g_locale_to_utf8(
        log_resolution(monitor->width, monitor->height), -1, NULL, NULL, NULL);

    gtk_text_buffer_set_text(buffer, selected_monitor, -1);

    g_object_set_data(G_OBJECT(app), "selected_monitor", (gpointer)monitor);

    if (g_object_get_data(G_OBJECT(app), "flowbox")) return;

    GtkWidget *vbox = g_object_get_data(G_OBJECT(app), "vbox");
    GtkWidget *flowbox = gtk_flow_box_new();
    g_object_set_data(G_OBJECT(app), "flowbox", flowbox);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), 10);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), 10);
    gtk_container_add(GTK_CONTAINER(vbox), flowbox);

    g_signal_connect(flowbox, "selected-children-changed",
                     G_CALLBACK(image_selected), app);

    if (image_paths) {
        for (int i = 0; i < number_of_images; i++) {
            GtkWidget *image = gtk_image_new();
            gtk_flow_box_insert(GTK_FLOW_BOX(flowbox), image, -1);
            show_image(image, image_paths[i], image_width);

            g_object_set_data(G_OBJECT(image), "image_path",
                              (gpointer)image_paths[i]);
        }

        g_object_set_data_full(G_OBJECT(app), "image_paths", image_paths,
                               free_image_paths);
    } else {
        g_print("No images found in %s\n", source_directory);
    }

    gtk_flow_box_set_sort_func(GTK_FLOW_BOX(flowbox), sort_flow_images, NULL,
                               NULL);
    gtk_widget_show_all(flowbox);
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    g_object_set_data(G_OBJECT(app), "vbox", vbox);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(vbox), menu_box);

    GtkWidget *button1 = gtk_button_new_with_label("Desktop background");
    gtk_box_pack_start(GTK_BOX(menu_box), button1, false, false, 10);

    GtkWidget *button2 = gtk_button_new_with_label("Lock screen background");
    gtk_box_pack_start(GTK_BOX(menu_box), button2, false, false, 10);

    GtkWidget *monitors_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(vbox), monitors_box);
    g_object_set_data(G_OBJECT(app), "monitors_box", monitors_box);

    Monitor *monitors;
    int number_of_monitors;
    list_monitors(&monitors, &number_of_monitors);

    for (int monitor_id = 0; monitor_id < number_of_monitors; monitor_id++) {
        Monitor *monitor = &monitors[monitor_id];

        gchar *resoulution = log_resolution(monitor->width, monitor->height);
        GtkWidget *button = gtk_button_new_with_label(resoulution);

        g_object_set_data(G_OBJECT(button), "monitor", (gpointer)monitor);
        g_signal_connect(button, "clicked", G_CALLBACK(display_images),
                         (gpointer)app);
        gtk_box_pack_start(GTK_BOX(monitors_box), button, false, false, 10);
    }

    g_object_set_data_full(G_OBJECT(app), "monitors", (gpointer)monitors,
                           free_monitors);

    GtkWidget *status_selected_monitor;
    status_selected_monitor = gtk_text_view_new();
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(status_selected_monitor));
    gtk_container_add(GTK_CONTAINER(monitors_box), status_selected_monitor);
    g_object_set_data(G_OBJECT(app), "status_selected_monitor", buffer);

    gtk_widget_show_all(window);
}

extern int initialize_application(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.webdevred.wpc", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
