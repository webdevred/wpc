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
#include "wallpaper.h"
#include "wallpaper_struct.h"

static gchar *log_resolution(int x, int y) {
    char *format = "%d x %d";
    int expected_length = snprintf(NULL, 0, format, x, y) + 1;
    gchar *output = malloc(expected_length * sizeof(gchar));
    sprintf(output, format, x, y);
    return output;
}

extern void g_free_config(gpointer data) {
    Config *config = (Config *)data;
    free_config(config);
}

static Wallpaper *get_flow_child_wallpaper(GtkFlowBoxChild *flow_child) {
    GtkWidget *image = gtk_flow_box_child_get_child(flow_child);

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

    printf("%s\n", menu_choice);
    if (g_strcmp0(menu_choice, "dm_background") == 0) {
        int *socket = g_object_get_data(G_OBJECT(app), "priv_socket");
        gchar *args = g_strdup_printf("%s %s", monitor->name, wallpaper->path);
        write(socket[1], args, strlen(args) + 1);
    } else {
        Config *config = g_object_get_data(G_OBJECT(app), "configuration");
        char *monitor_name = monitor->name;
        char *wallpaper_path = wallpaper->path;
        if (!config || !monitor_name || !wallpaper_path) {
            fprintf(stderr, "Error: Null input detected.\n");
            return;
        }

        MonitorBackgroundPair *monitors = config->monitors_with_backgrounds;
        int number_of_monitors = config->number_of_monitors;

        bool found = false;

        for (int i = 0; i < number_of_monitors; i++) {

            printf("Checking monitor %d: %s\n", i, monitors[i].name);

            if (monitors[i].name &&
                strcmp(monitors[i].name, monitor_name) == 0) {
                free(monitors[i].image_path);
                monitors[i].image_path =
                    strdup(wallpaper_path);
                found = true;
                break;
            }
        }

        if (!found) {
            MonitorBackgroundPair *new_list =
                realloc(monitors, (number_of_monitors + 1) *
                                      sizeof(MonitorBackgroundPair));
            if (!new_list) {
                fprintf(stderr, "Error: Memory allocation failed.\n");
                return;
            }

            config->monitors_with_backgrounds = new_list;
            MonitorBackgroundPair *new_monitor = &new_list[number_of_monitors];

            new_monitor->name =
                strdup(monitor_name);
            new_monitor->image_path =
                strdup(wallpaper_path);

            config->number_of_monitors++;
        }
        dump_config(config);

        Config *new_config = load_config();
        g_object_set_data_full(G_OBJECT(app), "configuration",
                               (gpointer)new_config, g_free_config);

        set_wallpapers();
    }
}

static void free_wallpapers(gpointer data) {
    Wallpaper *wallpapers = (Wallpaper *)data;
    if (wallpapers) {
        for (int i = 0; wallpapers[i].path != NULL; i++) {
            free(wallpapers[i].path);
        }
        free(wallpapers);
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
    int image_width = 600;
    int number_of_images;

    Config *config = g_object_get_data(G_OBJECT(app), "configuration");
    char *source_directory = config->source_directory;

    Wallpaper *wallpapers =
        list_wallpapers(source_directory, &number_of_images);

    Monitor *monitor = g_object_get_data(G_OBJECT(button), "monitor");

    GtkLabel *status_label =
        g_object_get_data(G_OBJECT(app), "status_selected_monitor");

    GtkButton *button_menu_choice =
        g_object_get_data(G_OBJECT(app), "menu_choice");
    const gchar *status =
        g_strconcat(gtk_button_get_label(button_menu_choice), " > ",
                    gtk_button_get_label(button), NULL);

    gtk_label_set_label(status_label, status);

    if (monitor) {
        g_object_set_data(G_OBJECT(app), "selected_monitor", (gpointer)monitor);
    } else {
        g_object_set_data(G_OBJECT(app), "selected_monitor", NULL);
    }

    if (g_object_get_data(G_OBJECT(app), "flowbox")) return;

    GtkWidget *vbox = g_object_get_data(G_OBJECT(app), "vbox");
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_box_append(GTK_BOX(vbox), scrolled_window);

    GtkWidget *flowbox = gtk_flow_box_new();
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flowbox), 4);
    gtk_widget_set_vexpand(GTK_WIDGET(flowbox), true);
    g_object_set_data(G_OBJECT(app), "flowbox", flowbox);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), 10);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window),
                                  flowbox);

    g_signal_connect(flowbox, "selected-children-changed",
                     G_CALLBACK(image_selected), app);

    if (wallpapers) {
        for (int i = 0; i < number_of_images; i++) {
            GtkWidget *image = gtk_image_new_from_file(wallpapers[i].path);
            gtk_flow_box_insert(GTK_FLOW_BOX(flowbox), image, -1);
            gtk_widget_set_visible(image, true);
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
    gtk_widget_set_visible(scrolled_window, true);
    gtk_widget_set_visible(flowbox, true);
}

static void wm_show_monitors(GtkButton *button, gpointer user_data) {
    (void)button;
    GtkApplication *app = GTK_APPLICATION(user_data);

    GtkBox *dm_monitors_box =
        g_object_get_data(G_OBJECT(app), "dm_monitors_box");
    gtk_widget_set_visible(GTK_WIDGET(dm_monitors_box), false);

    GtkBox *wm_monitors_box =
        g_object_get_data(G_OBJECT(app), "wm_monitors_box");
    gtk_widget_set_visible(GTK_WIDGET(wm_monitors_box), true);

    g_object_set_data(G_OBJECT(app), "menu_choice", (gpointer)button);
}

static void dm_show_monitors(GtkButton *button, gpointer user_data) {
    (void)button;
    GtkApplication *app = GTK_APPLICATION(user_data);
    GtkBox *dm_monitors_box =
        g_object_get_data(G_OBJECT(app), "dm_monitors_box");
    gtk_widget_set_visible(GTK_WIDGET(dm_monitors_box), true);

    GtkBox *wm_monitors_box =
        g_object_get_data(G_OBJECT(app), "wm_monitors_box");
    gtk_widget_set_visible(GTK_WIDGET(wm_monitors_box), false);

    g_object_set_data(G_OBJECT(app), "menu_choice", (gpointer)button);
}

void setup_dm_monitors_box(GtkApplication *app, GtkWidget *menu_box,
                           GtkWidget *monitors_box) {
    GtkWidget *button_dm_list_monitors =
        gtk_button_new_with_label("Lock screen background");
    g_signal_connect(button_dm_list_monitors, "clicked",
                     G_CALLBACK(dm_show_monitors), (gpointer)app);
    g_object_set_data(G_OBJECT(button_dm_list_monitors), "name",
                      "dm_background");
    gtk_box_append(GTK_BOX(menu_box), button_dm_list_monitors);

    Monitor *primary_monitor, *secondary_monitor;
    int number_of_other_monitors;

    primary_monitor = malloc(sizeof(Monitor));
    secondary_monitor = malloc(sizeof(Monitor));

    dm_list_monitors(primary_monitor, secondary_monitor,
                     &number_of_other_monitors);

    GtkWidget *button_primary_monitor = gtk_button_new_with_label(
        g_strdup_printf("Primary monitor (%d x %d)", primary_monitor->width,
                        primary_monitor->height));

    g_object_set_data_full(G_OBJECT(button_primary_monitor), "monitor",
                           (gpointer)primary_monitor, free_monitor);

    g_signal_connect(button_primary_monitor, "clicked", G_CALLBACK(show_images),
                     (gpointer)app);

    gtk_box_append(GTK_BOX(monitors_box), button_primary_monitor);

    if (number_of_other_monitors == 1) {
        GtkWidget *button_secondary_monitor =
            gtk_button_new_with_label(g_strdup_printf(
                "Secondary monitor (%d x %d)", secondary_monitor->width,
                secondary_monitor->height));

        g_object_set_data_full(G_OBJECT(button_secondary_monitor), "monitor",
                               (gpointer)secondary_monitor, free_monitor);

        g_signal_connect(button_secondary_monitor, "clicked",
                         G_CALLBACK(show_images), (gpointer)app);

        gtk_box_append(GTK_BOX(monitors_box), button_secondary_monitor);
    } else if (number_of_other_monitors > 1) {
        GtkWidget *button_other_monitors = gtk_button_new_with_label(
            g_strdup_printf("Other monitors (%d)", number_of_other_monitors));

        g_object_set_data_full(G_OBJECT(button_other_monitors), "monitor",
                               (gpointer)secondary_monitor, free_monitor);

        g_signal_connect(button_other_monitors, "clicked",
                         G_CALLBACK(show_images), (gpointer)app);

        gtk_box_append(GTK_BOX(monitors_box), button_other_monitors);
    } else {
        free(secondary_monitor);
    }
}

void setup_wm_monitors_box(GtkApplication *app, GtkWidget *menu_box,
                           GtkWidget *monitors_box) {
    GtkWidget *button_wm_list_monitors =
        gtk_button_new_with_label("Desktop background");
    g_object_set_data(G_OBJECT(button_wm_list_monitors), "name",
                      "wm_background");
    g_signal_connect(button_wm_list_monitors, "clicked",
                     G_CALLBACK(wm_show_monitors), (gpointer)app);
    gtk_box_append(GTK_BOX(menu_box), button_wm_list_monitors);

    int number_of_monitors;
    Monitor *monitors = wm_list_monitors(&number_of_monitors);

    /* destroy_all_widgets(monitors_box); */

    for (int monitor_id = 0; monitor_id < number_of_monitors; monitor_id++) {
        Monitor *monitor = &monitors[monitor_id];

        GtkWidget *button = gtk_button_new_with_label(g_strdup_printf(
            "%s - %d x %d", monitor->name, monitor->width, monitor->height));

        g_object_set_data(G_OBJECT(button), "monitor", (gpointer)monitor);
        g_signal_connect(button, "clicked", G_CALLBACK(show_images),
                         (gpointer)app);
        gtk_box_append(GTK_BOX(monitors_box), button);
    }

    g_object_set_data_full(G_OBJECT(app), "monitors", (gpointer)monitors,
                           free_monitors);
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;

    Config *config = load_config();
    g_object_set_data_full(G_OBJECT(app), "configuration", (gpointer)config,
                           g_free_config);

    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    g_object_set_data(G_OBJECT(app), "vbox", vbox);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(vbox), menu_box);

    GtkWidget *wm_monitors_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(vbox), wm_monitors_box);
    g_object_set_data(G_OBJECT(app), "wm_monitors_box", wm_monitors_box);
    gtk_widget_set_visible(GTK_WIDGET(wm_monitors_box), false);

    GtkWidget *dm_monitors_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(vbox), dm_monitors_box);
    g_object_set_data(G_OBJECT(app), "dm_monitors_box", dm_monitors_box);
    gtk_widget_set_visible(GTK_WIDGET(dm_monitors_box), false);

    setup_wm_monitors_box(app, menu_box, wm_monitors_box);
    setup_dm_monitors_box(app, menu_box, dm_monitors_box);

    GtkWidget *status_selected_monitor = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(status_selected_monitor), false);
    gtk_box_append(GTK_BOX(menu_box), status_selected_monitor);
    g_object_set_data(G_OBJECT(app), "status_selected_monitor",
                      status_selected_monitor);

    gtk_widget_set_visible(window, true);
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
