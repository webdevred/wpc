#include <glib.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#include "filesystem.h"
#include "gui.h"
#include "lightdm.h"
#include "monitors.h"
#include "resolution_scaling.h"
#include "wallpaper_struct.h"

static gchar *log_resolution(int x, int y) {
    char *format = "%d x %d";
    int expected_length = snprintf(NULL, 0, format, x, y) + 1;
    gchar *output = malloc(expected_length * sizeof(gchar));
    sprintf(output, format, x, y);
    return output;
}
static void show_image(GtkWidget *image, const gchar *image_path,
                       const int new_image_width) {
    gtk_image_set_from_file(GTK_IMAGE(image), image_path);
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

static Wallpaper *get_flow_child_wallpaper(GtkFlowBoxChild *flow_child) {
    GtkWidget *image = gtk_bin_get_child(GTK_BIN(flow_child));

    Wallpaper *wallpaper = g_object_get_data(G_OBJECT(image), "wallpaper");
    return wallpaper;
}

static void image_selected(GtkFlowBox *flowbox, gpointer user_data) {
    GtkApplication *app = GTK_APPLICATION(user_data);
    Monitor *monitor = g_object_get_data(G_OBJECT(app), "selected_monitor");

    GList *flowbox_children = gtk_flow_box_get_selected_children(flowbox);
    GtkFlowBoxChild *selected_children = flowbox_children->data;

    Wallpaper *wallpaper = get_flow_child_wallpaper(selected_children);

    g_print("Clicked image %s Selected monitor: %dx%d\n", wallpaper->path,
            monitor->width, monitor->height);

    GtkButton *button_menu_choice =
        g_object_get_data(G_OBJECT(app), "menu_choice");

    gchar *menu_choice =
        g_object_get_data(G_OBJECT(button_menu_choice), "name");

    if (g_strcmp0(menu_choice, "dm_background")) {
        int *socket = g_object_get_data(G_OBJECT(app), "priv_socket");
        gchar *args = g_strdup_printf("%s %s", wallpaper->path, monitor->name);
        write(socket[1], args, strlen(args) + 1);
    }
}

static void free_wallpapers(gpointer data) {
    Wallpaper *wallpapers = (Wallpaper *)data;
    if (wallpapers) {
        // Iterate over the wallpapers array until we hit the NULL-terminated
        // path
        for (int i = 0; wallpapers[i].path != NULL; i++) {
            free(wallpapers[i].path); // Free each wallpaper's path
        }
        free(wallpapers); // Free the wallpaper array itself
    }
}

static void free_monitor(gpointer data) {
    Monitor *primary_monitor = (Monitor *)data;
    free(primary_monitor);
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
    Wallpaper *wallpaper1 = get_flow_child_wallpaper(child1);
    Wallpaper *wallpaper2 = get_flow_child_wallpaper(child2);

    gchar *image_path1 = wallpaper1->path;
    gchar *image_path2 = wallpaper2->path;
    return g_strcmp0(image_path1, image_path2);
}

static void show_images(GtkButton *button, GtkApplication *app) {
    int image_width = 500;
    int number_of_images;

    Config *config = g_object_get_data(G_OBJECT(app), "configuration");
    char *source_directory = config->source_directory;

    Wallpaper *wallpapers =
        list_wallpapers(source_directory, &number_of_images);

    Monitor *monitor = g_object_get_data(G_OBJECT(button), "monitor");

    GtkTextBuffer *buffer =
        g_object_get_data(G_OBJECT(app), "status_selected_monitor");

    GtkButton *button_menu_choice =
        g_object_get_data(G_OBJECT(app), "menu_choice");
    const gchar *status =
        g_strconcat(gtk_button_get_label(button_menu_choice), " > ",
                    gtk_button_get_label(button), NULL);

    gtk_text_buffer_set_text(buffer, status, -1);

    if (monitor) {
        g_object_set_data(G_OBJECT(app), "selected_monitor", (gpointer)monitor);
    } else {
        g_object_set_data(G_OBJECT(app), "selected_monitor", NULL);
    }

    if (g_object_get_data(G_OBJECT(app), "flowbox")) return;

    GtkWidget *vbox = g_object_get_data(G_OBJECT(app), "vbox");
    GtkWidget *flowbox = gtk_flow_box_new();
    g_object_set_data(G_OBJECT(app), "flowbox", flowbox);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), 10);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), 10);
    gtk_container_add(GTK_CONTAINER(vbox), flowbox);

    g_signal_connect(flowbox, "selected-children-changed",
                     G_CALLBACK(image_selected), app);

    if (wallpapers) {
        for (int i = 0; i < number_of_images; i++) {
            GtkWidget *image = gtk_image_new();
            gtk_flow_box_insert(GTK_FLOW_BOX(flowbox), image, -1);

            show_image(image, wallpapers[i].path, image_width);

            g_object_set_data(G_OBJECT(image), "wallpaper",
                              (gpointer)&wallpapers[i]);
        }

        g_object_set_data_full(G_OBJECT(app), "wallpapers",
                               (gpointer)wallpapers, free_wallpapers);
    } else {
        g_print("No images found in %s\n", source_directory);
    }

    gtk_flow_box_set_sort_func(GTK_FLOW_BOX(flowbox), sort_flow_images, NULL,
                               NULL);
    gtk_widget_show_all(flowbox);
}

static void wm_show_monitors(GtkButton *button, gpointer user_data) {
    (void)button;
    GtkApplication *app = GTK_APPLICATION(user_data);
    GtkWidget *monitors_box = g_object_get_data(G_OBJECT(app), "monitors_box");
    int number_of_monitors;
    Monitor *monitors = wm_list_monitors(&number_of_monitors);

    destroy_all_widgets(monitors_box);

    g_object_set_data(G_OBJECT(app), "menu_choice", (gpointer)button);

    for (int monitor_id = 0; monitor_id < number_of_monitors; monitor_id++) {
        Monitor monitor = monitors[monitor_id];

        gchar *resoulution = log_resolution(monitor.width, monitor.height);
        GtkWidget *button = gtk_button_new_with_label(resoulution);

        g_object_set_data(G_OBJECT(button), "monitor", (gpointer)&monitor);
        g_signal_connect(button, "clicked", G_CALLBACK(show_images),
                         (gpointer)app);
        gtk_box_pack_start(GTK_BOX(monitors_box), button, false, false, 10);
    }

    g_object_set_data_full(G_OBJECT(app), "monitors", (gpointer)monitors,
                           free_monitors);
    gtk_widget_show_all(monitors_box);
}

static void dm_show_monitors(GtkButton *button, gpointer user_data) {
    (void)button;
    GtkApplication *app = GTK_APPLICATION(user_data);
    GtkWidget *monitors_box = g_object_get_data(G_OBJECT(app), "monitors_box");
    Monitor *primary_monitor, *secondary_monitor;
    int number_of_other_monitors;

    primary_monitor = malloc(sizeof(Monitor));
    secondary_monitor = malloc(sizeof(Monitor));

    destroy_all_widgets(monitors_box);

    g_object_set_data(G_OBJECT(app), "menu_choice", (gpointer)button);

    dm_list_monitors(primary_monitor, secondary_monitor,
                     &number_of_other_monitors);

    GtkWidget *button_primary_monitor = gtk_button_new_with_label(
        g_strdup_printf("Primary monitor (%d x %d)", primary_monitor->width,
                        primary_monitor->height));

    g_object_set_data_full(G_OBJECT(button_primary_monitor), "monitor",
                           (gpointer)primary_monitor, free_monitor);

    g_signal_connect(button_primary_monitor, "clicked", G_CALLBACK(show_images),
                     (gpointer)app);

    gtk_box_pack_start(GTK_BOX(monitors_box), button_primary_monitor, false,
                       false, 10);

    if (number_of_other_monitors == 1) {
        GtkWidget *button_secondary_monitor =
            gtk_button_new_with_label(g_strdup_printf(
                "Secondary monitor (%d x %d)", secondary_monitor->width,
                secondary_monitor->height));

        g_object_set_data_full(G_OBJECT(button_secondary_monitor), "monitor",
                               (gpointer)secondary_monitor, free_monitor);

        g_signal_connect(button_secondary_monitor, "clicked",
                         G_CALLBACK(show_images), (gpointer)app);

        gtk_box_pack_start(GTK_BOX(monitors_box), button_secondary_monitor,
                           false, false, 10);
    } else if (number_of_other_monitors > 1) {
        GtkWidget *button_other_monitors = gtk_button_new_with_label(
            g_strdup_printf("Other monitors (%d)", number_of_other_monitors));

        g_object_set_data_full(G_OBJECT(button_other_monitors), "monitor",
                               (gpointer)secondary_monitor, free_monitor);

        g_signal_connect(button_other_monitors, "clicked",
                         G_CALLBACK(show_images), (gpointer)app);

        gtk_box_pack_start(GTK_BOX(monitors_box), button_other_monitors, false,
                           false, 10);
    } else {
        free(secondary_monitor);
    }

    gtk_widget_show_all(monitors_box);
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;

    Config *config = load_config();
    g_object_set_data_full(G_OBJECT(app), "configuration", (gpointer)config,
                           free_config);

    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    g_object_set_data(G_OBJECT(app), "vbox", vbox);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(vbox), menu_box);

    GtkWidget *button_wm_list_monitors =
        gtk_button_new_with_label("Desktop background");
    g_object_set_data(G_OBJECT(button_wm_list_monitors), "name",
                      "wm_background");
    g_signal_connect(button_wm_list_monitors, "clicked",
                     G_CALLBACK(wm_show_monitors), (gpointer)app);
    gtk_box_pack_start(GTK_BOX(menu_box), button_wm_list_monitors, false, false,
                       10);
    wm_show_monitors(GTK_BUTTON(button_wm_list_monitors), (gpointer)app);

    GtkWidget *button_dm_list_monitors =
        gtk_button_new_with_label("Lock screen background");
    g_signal_connect(button_dm_list_monitors, "clicked",
                     G_CALLBACK(dm_show_monitors), (gpointer)app);
    g_object_set_data(G_OBJECT(button_wm_list_monitors), "name",
                      "dm_background");
    gtk_box_pack_start(GTK_BOX(menu_box), button_dm_list_monitors, false, false,
                       10);

    GtkWidget *monitors_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(vbox), monitors_box);
    g_object_set_data(G_OBJECT(app), "monitors_box", monitors_box);

    GtkWidget *status_selected_monitor = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(status_selected_monitor), false);
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(status_selected_monitor));
    gtk_container_add(GTK_CONTAINER(menu_box), status_selected_monitor);
    g_object_set_data(G_OBJECT(app), "status_selected_monitor", buffer);

    gtk_widget_show_all(window);
}

extern int initialize_application(int argc, char **argv, int *socket) {
    GtkApplication *app;
    int status;
    app = gtk_application_new("org.webdevred.wpc", G_APPLICATION_DEFAULT_FLAGS);
    g_object_set_data(G_OBJECT(app), "priv_socket", (gpointer)socket);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
