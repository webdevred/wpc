#include <glib.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#include "filesystem.h"
#include "gui.h"
#include "imagemagick.h"
#include "monitors.h"
#include "resolution_scaling.h"
#include "wallpaper.h"

#ifdef WPC_ENABLE_HELPER
#include "lightdm.h"
#endif

typedef enum { DM_BACKGROUND = 0, WM_BACKGROUND } AppTab;

static Wallpaper *get_flow_child_wallpaper(GtkFlowBoxChild *flow_child) {
    GtkWidget *image = gtk_flow_box_child_get_child(flow_child);
    Wallpaper *wallpaper = g_object_get_data(G_OBJECT(image), "wallpaper");
    return wallpaper;
}

static void image_selected(GtkFlowBox *flowbox, gpointer user_data) {
    GtkApplication *app = GTK_APPLICATION(user_data);
    Monitor *monitor = g_object_get_data(G_OBJECT(app), "selected_monitor");

    GList *flowbox_children = gtk_flow_box_get_selected_children(flowbox);

    if (!flowbox_children) return;

    GtkFlowBoxChild *selected_children = flowbox_children->data;

    Wallpaper *wallpaper = get_flow_child_wallpaper(selected_children);

    g_info("Clicked image %s Selected monitor: %dx%d", wallpaper->path,
           monitor->width, monitor->height);

#ifdef WPC_ENABLE_HELPER
    GtkButton *button_menu_choice =
        g_object_get_data(G_OBJECT(app), "menu_choice");

    AppTab *menu_choice =
        g_object_get_data(G_OBJECT(button_menu_choice), "name");

    if (*menu_choice == DM_BACKGROUND) {
        lightdm_set_background(wallpaper, monitor);
    } else {
#endif
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

        g_info("trying to update monitor in existing configuration");
        for (int i = 0; i < number_of_monitors; i++) {
            if (monitors[i].name &&
                strcmp(monitors[i].name, monitor_name) == 0) {
                free(monitors[i].image_path);
                monitors[i].image_path = strdup(wallpaper_path);
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

            new_monitor->name = strdup(monitor_name);
            new_monitor->image_path = strdup(wallpaper_path);

            config->number_of_monitors++;
        }
        dump_config(config);

        set_wallpapers();
#ifdef WPC_ENABLE_HELPER
    }
#endif
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

static void show_images_src_dir(GtkApplication *app) {
    Config *config = g_object_get_data(G_OBJECT(app), "configuration");
    char *source_directory = config->source_directory;
    GtkWidget *flowbox = g_object_get_data(G_OBJECT(app), "flowbox");

    ArrayWrapper *old_wp_arr_wrapper =
        g_object_get_data(G_OBJECT(app), "wallpapers");
    if (old_wp_arr_wrapper) {
        free_wallpapers(old_wp_arr_wrapper);
        g_object_set_data(G_OBJECT(app), "wallpapers", NULL);
    }

    gtk_flow_box_remove_all(GTK_FLOW_BOX(flowbox));

    ArrayWrapper *wp_arr_wrapper = list_wallpapers(source_directory);
    Wallpaper *wallpapers = (Wallpaper *)wp_arr_wrapper->data;

    if (wallpapers) {
        gtk_flow_box_set_sort_func(GTK_FLOW_BOX(flowbox), NULL, NULL, NULL);
        unsigned int i;
        for (i = 0; i < wp_arr_wrapper->amount_used; i++) {
            GtkWidget *image = gtk_image_new_from_file(wallpapers[i].path);
            gtk_flow_box_insert(GTK_FLOW_BOX(flowbox), image, -1);
            gtk_widget_set_visible(image, true);
            g_object_set_data(G_OBJECT(image), "wallpaper",
                              (gpointer)&wallpapers[i]);
        }

        gtk_flow_box_set_sort_func(GTK_FLOW_BOX(flowbox), sort_flow_images,
                                   NULL, NULL);

        g_object_set_data(G_OBJECT(app), "wallpapers",
                          (gpointer)wp_arr_wrapper);
    } else {
        g_print("No images found in %s\n", source_directory);
    }
}

static void on_option_selected(GtkDropDown *dropdown, GParamSpec *spec,
                               gpointer user_data) {
    (void)spec;
    GtkApplication *app = GTK_APPLICATION(user_data);
    Config *config = g_object_get_data(G_OBJECT(app), "configuration");
    guint selected_index = gtk_drop_down_get_selected(dropdown);

    BgMode bg_mode = selected_index;

    GtkButton *button_menu_choice =
        g_object_get_data(G_OBJECT(app), "menu_choice");

    AppTab *menu_choice =
        g_object_get_data(G_OBJECT(button_menu_choice), "name");

    if (*menu_choice == WM_BACKGROUND) {
        MonitorBackgroundPair **monitor_bgs =
            &(config->monitors_with_backgrounds);
        Monitor *monitor = g_object_get_data(G_OBJECT(app), "selected_monitor");
        if (!monitor && selected_index == BG_MODE_NOT_SET) return;
        for (int i = 0; i < config->number_of_monitors; i++) {
            if (strcmp((*monitor_bgs)[i].name, monitor->name) == 0) {
                (*monitor_bgs)[i].bg_mode = bg_mode;
            }
        }

        dump_config(config);
        set_wallpapers();
    }
}

static void show_images(GtkButton *button, GtkApplication *app) {
    Monitor *monitor = g_object_get_data(G_OBJECT(button), "monitor");

    GtkLabel *status_label =
        g_object_get_data(G_OBJECT(app), "status_selected_monitor");

    GtkButton *button_menu_choice =
        g_object_get_data(G_OBJECT(app), "menu_choice");
    const gchar *status =
        g_strconcat(gtk_button_get_label(button_menu_choice), " > ",
                    gtk_button_get_label(button), NULL);

    gtk_label_set_label(status_label, status);

    GtkWidget *bg_mode_dropdown =
        g_object_get_data(G_OBJECT(app), "bg_mode_dropdown");
    gtk_widget_set_visible(GTK_WIDGET(bg_mode_dropdown), true);

    BgMode bg_mode = BG_MODE_NOT_SET;

    AppTab *menu_choice =
        g_object_get_data(G_OBJECT(button_menu_choice), "name");

    Config *config = g_object_get_data(G_OBJECT(app), "configuration");
    if (menu_choice) {
        if (*menu_choice == WM_BACKGROUND && monitor) {
            for (gushort i = 0; i < config->number_of_monitors; i++) {
                MonitorBackgroundPair *monitor_bg =
                    &(config->monitors_with_backgrounds[i]);
                if (strcmp(monitor_bg->name, monitor->name) == 0) {
                    bg_mode = monitor_bg->bg_mode;
                }
            }
        }
    }

    if (monitor) {
        g_object_set_data(G_OBJECT(app), "selected_monitor", (gpointer)monitor);
    } else {
        g_object_set_data(G_OBJECT(app), "selected_monitor", NULL);
    }

    gtk_drop_down_set_selected(GTK_DROP_DOWN(bg_mode_dropdown), (guint)bg_mode);

    if (!g_object_get_data(G_OBJECT(app), "flowbox")) {
        GtkWidget *vbox = g_object_get_data(G_OBJECT(app), "vbox");
        GtkWidget *scrolled_window = gtk_scrolled_window_new();
        gtk_box_append(GTK_BOX(vbox), scrolled_window);

        GtkWidget *flowbox = gtk_flow_box_new();
        gtk_widget_set_vexpand(GTK_WIDGET(flowbox), true);
        g_object_set_data(G_OBJECT(app), "flowbox", flowbox);
        gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), 10);
        gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), 10);
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window),
                                      flowbox);

        g_signal_connect(flowbox, "selected-children-changed",
                         G_CALLBACK(image_selected), app);

        gtk_widget_set_visible(scrolled_window, true);
        gtk_widget_set_visible(flowbox, true);
        show_images_src_dir(app);
    }
}

static void show_monitors(GtkApplication *app) {
    GtkWidget *flowbox = g_object_get_data(G_OBJECT(app), "flowbox");
    if (flowbox) gtk_flow_box_unselect_all(GTK_FLOW_BOX(flowbox));

    GtkBox *monitors_box = g_object_get_data(G_OBJECT(app), "monitors_box");
    if (!gtk_widget_is_visible(GTK_WIDGET(monitors_box)))
        gtk_widget_set_visible(GTK_WIDGET(monitors_box), true);
}

static void wm_show_monitors(GtkButton *button, gpointer user_data) {
    (void)button;
    GtkApplication *app = GTK_APPLICATION(user_data);

    show_monitors(app);
    g_object_set_data(G_OBJECT(app), "menu_choice", (gpointer)button);
}

#ifdef WPC_ENABLE_HELPER
static void dm_show_monitors(GtkButton *button, gpointer user_data) {
    (void)button;
    GtkApplication *app = GTK_APPLICATION(user_data);

    show_monitors(app);

    g_object_set_data(G_OBJECT(app), "menu_choice", (gpointer)button);
}

void setup_dm_monitors_button(GtkApplication *app, GtkWidget *menu_box) {
    GtkWidget *button_dm_show_monitors =
        gtk_button_new_with_label("Lock screen background");
    AppTab *app_tab = malloc(sizeof(app_tab));
    *app_tab = DM_BACKGROUND;
    g_object_set_data_full(G_OBJECT(button_dm_show_monitors), "name",
                           (gpointer)app_tab, g_free);
    g_signal_connect(button_dm_show_monitors, "clicked",
                     G_CALLBACK(dm_show_monitors), (gpointer)app);
    gtk_box_append(GTK_BOX(menu_box), button_dm_show_monitors);
}
#endif

void setup_wm_monitors_button(GtkApplication *app, GtkWidget *menu_box) {
    GtkWidget *button_wm_show_monitors =
        gtk_button_new_with_label("Desktop background");
    AppTab *app_tab = malloc(sizeof(app_tab));
    *app_tab = WM_BACKGROUND;
    g_object_set_data_full(G_OBJECT(button_wm_show_monitors), "name",
                           (gpointer)app_tab, g_free);
    g_signal_connect(button_wm_show_monitors, "clicked",
                     G_CALLBACK(wm_show_monitors), (gpointer)app);
    gtk_box_append(GTK_BOX(menu_box), button_wm_show_monitors);
}

static void storage_dir_chosen(GObject *source_object, GAsyncResult *res,
                               gpointer user_data) {
    GtkApplication *app = GTK_APPLICATION(user_data);
    Config *config = g_object_get_data(G_OBJECT(app), "configuration");

    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    GFile *dir;
    GError *err = NULL;
    char *new_src_dir;
    dir = gtk_file_dialog_select_folder_finish(dialog, res, &err);

    if (dir == NULL && err->code == 2) {
        return;
    }

    if (dir == NULL && err->code != 2) {
        g_warning("Error code: %d Error: %s", err->code, err->message);
        return;
    }
    new_src_dir = g_file_get_path(dir);

    update_source_directory(config, new_src_dir);
    dump_config(config);
    show_images_src_dir(app);
}

static void choose_source_dir(GtkWidget *button, gpointer user_data) {
    (void)button;
    GtkApplication *app = GTK_APPLICATION(user_data);
    Config *config = g_object_get_data(G_OBJECT(app), "configuration");
    GtkWindow *window = gtk_application_get_active_window(app);
    GFile *initial_src_dir = g_file_new_for_path(config->source_directory);
    GtkFileDialog *dialog = gtk_file_dialog_new();
    dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_initial_folder(dialog, initial_src_dir);
    gtk_file_dialog_select_folder(dialog, GTK_WINDOW(window), NULL,
                                  storage_dir_chosen, (gpointer)app);
    g_object_unref(dialog);
}

/* Recursively traverse the widget tree and “free” (unparent) widgets.
   (Typically GTK will free widget memory when the widget hierarchy is
   destroyed.) */
void free_child_widget(GtkWidget *widget) {
    GtkWidget *sub_widget;
    while ((sub_widget = gtk_widget_get_first_child(widget)) != NULL) {
        g_print("Freeing child widget: %p\n", sub_widget);
        free_child_widget(sub_widget); // Recursively clean up child widgets
    }
    gtk_widget_unparent(widget);
    g_print("Freeing widget: %p\n", widget);
}

void free_dynamic_widgets(GtkApplication *app) {
    GtkWidget *vbox = g_object_get_data(G_OBJECT(app), "vbox");
    if (vbox) {
        free_child_widget(vbox);
        gtk_widget_unparent(vbox);
        g_object_set_data(G_OBJECT(app), "vbox", NULL);
    }
}
static gboolean on_window_close(GtkWindow *window, gpointer user_data) {
    (void)window;
    GtkApplication *app = GTK_APPLICATION(user_data);

    Config *config = g_object_get_data(G_OBJECT(app), "configuration");
    if (config) {
        free_config(config);
        g_object_set_data(G_OBJECT(app), "configuration", NULL);
    }

    ArrayWrapper *mon_arr_wrapper =
        g_object_get_data(G_OBJECT(app), "monitors");
    if (mon_arr_wrapper) {
        free_monitors(mon_arr_wrapper);
        g_object_set_data(G_OBJECT(app), "monitors", NULL);
    }

    ArrayWrapper *wp_arr_wrapper =
        g_object_get_data(G_OBJECT(app), "wallpapers");
    if (wp_arr_wrapper) {
        free_wallpapers(wp_arr_wrapper);

        g_object_set_data(G_OBJECT(app), "wallpapers", NULL);
    }

    g_object_set_data(G_OBJECT(app), "selected_monitor", NULL);
    g_object_set_data(G_OBJECT(app), "menu_choice", NULL);
    g_object_set_data(G_OBJECT(app), "status_selected_monitor", NULL);
    g_object_set_data(G_OBJECT(app), "vbox", NULL);
    g_object_set_data(G_OBJECT(app), "flowbox", NULL);
    g_object_set_data(G_OBJECT(app), "monitors_box", NULL);

    free_dynamic_widgets(app);

    return FALSE;
}

const gchar *bg_mode_to_display_string(BgMode type) {
    switch (type) {
    case BG_MODE_TILE:
        return "Tile";
    case BG_MODE_CENTER:
        return "Center";
    case BG_MODE_SCALE:
        return "Scale";
    case BG_MODE_MAX:
        return "Max";
    case BG_MODE_FILL:
        return "Fill";
    default:
        return "(Not set)";
    }
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;

    Config *config = load_config();
    g_object_set_data(G_OBJECT(app), "configuration", (gpointer)config);

    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
    g_signal_connect(window, "close-request", G_CALLBACK(on_window_close), app);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    g_object_set_data(G_OBJECT(app), "vbox", vbox);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(vbox), menu_box);

    GtkWidget *monitors_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(vbox), monitors_box);
    g_object_set_data(G_OBJECT(app), "monitors_box", monitors_box);
    gtk_widget_set_visible(GTK_WIDGET(monitors_box), false);

    ArrayWrapper *mon_arr_wrapper = list_monitors(true);
    Monitor *monitors = (Monitor *)mon_arr_wrapper->data;

    unsigned int monitor_id;
    for (monitor_id = 0; monitor_id < mon_arr_wrapper->amount_used;
         monitor_id++) {
        Monitor *monitor = &monitors[monitor_id];

        GtkWidget *button = gtk_button_new_with_label(g_strdup_printf(
            "%s - %d x %d", monitor->name, monitor->width, monitor->height));

        g_object_set_data(G_OBJECT(button), "monitor", (gpointer)monitor);
        g_signal_connect(button, "clicked", G_CALLBACK(show_images),
                         (gpointer)app);
        gtk_box_append(GTK_BOX(monitors_box), button);
    }

    g_object_set_data(G_OBJECT(app), "monitors", (gpointer)mon_arr_wrapper);

    setup_wm_monitors_button(app, menu_box);
#ifdef WPC_ENABLE_HELPER
    setup_dm_monitors_button(app, menu_box);
#endif

    GtkWidget *button_settings = gtk_button_new_with_label("Settings");
    g_signal_connect(button_settings, "clicked", G_CALLBACK(choose_source_dir),
                     (gpointer)app);
    gtk_box_append(GTK_BOX(menu_box), button_settings);

    GPtrArray *array;
    GtkStringList *string_list;

    array = g_ptr_array_new_with_free_func(g_free);
    for (int i = 0; i < BG_MODE_NOT_SET; i++) {
        int i2 = i == 0 ? BG_MODE_NOT_SET : i;
        g_ptr_array_add(array, g_strdup(bg_mode_to_display_string(i2)));
    }
    g_ptr_array_add(array, NULL);
    string_list = gtk_string_list_new(
        (const gchar *const *)g_ptr_array_free(array, FALSE));

    GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(string_list), NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown), 0);
    gtk_widget_set_visible(GTK_WIDGET(dropdown), false);
    gtk_box_append(GTK_BOX(menu_box), dropdown);
    g_signal_connect(dropdown, "notify::selected",
                     G_CALLBACK(on_option_selected), (gpointer)app);
    g_object_set_data(G_OBJECT(app), "bg_mode_dropdown", dropdown);

    GtkWidget *status_selected_monitor = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(status_selected_monitor), false);
    gtk_box_append(GTK_BOX(menu_box), status_selected_monitor);
    g_object_set_data(G_OBJECT(app), "status_selected_monitor",
                      status_selected_monitor);

    gtk_widget_set_visible(window, true);
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
