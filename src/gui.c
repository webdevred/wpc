#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "filesystem.h"
#include "gui.h"
#include "monitors.h"
#include "utils.h"
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
        GtkWidget *bg_mode_dropdown =
            g_object_get_data(G_OBJECT(app), "bg_mode_dropdown");
        guint selected_index =
            gtk_drop_down_get_selected(GTK_DROP_DOWN(bg_mode_dropdown));
        lightdm_set_background(wallpaper, monitor, selected_index);
    } else {
#endif
        Config *config = g_object_get_data(G_OBJECT(app), "configuration");
        char *monitor_name = monitor->name;
        char *wallpaper_path = wallpaper->path;
        if (!config || !monitor_name || !wallpaper_path) {
            fprintf(stderr, "Error: Null input detected.\n");
            return;
        }

        if (is_empty_string(wallpaper->path)) return;

        if (monitor->belongs_to_config) {
            ConfigMonitor *config_monitor =
                &config->monitors_with_backgrounds[monitor->config_id];
            config_monitor->image_path = g_strdup(wallpaper_path);
            monitor->wallpaper = wallpaper;
        } else {
            ConfigMonitor *monitors = config->monitors_with_backgrounds;
            int number_of_monitors = config->number_of_monitors;

            ConfigMonitor *new_list = realloc(
                monitors, (number_of_monitors + 1) * sizeof(ConfigMonitor));
            if (!new_list) {
                fprintf(stderr, "Error: Memory allocation failed.\n");
                return;
            }

            config->monitors_with_backgrounds = new_list;
            ConfigMonitor *new_bmp = &new_list[number_of_monitors];

            new_bmp->name = g_strdup(monitor_name);
            new_bmp->image_path = g_strdup(wallpaper_path);
            new_bmp->bg_fallback_color = g_strdup("");
            new_bmp->valid_bg_fallback_color = g_strdup("");

            monitor->belongs_to_config = true;
            monitor->config_id = number_of_monitors;
            monitor->wallpaper = wallpaper;
            config->number_of_monitors++;
        }
        dump_config(config);
        set_wallpapers(config);
#ifdef WPC_ENABLE_HELPER
    }
#endif
}

static gint sort_flow_images(GtkFlowBoxChild *child1, GtkFlowBoxChild *child2,
                             gpointer user_data) {
    (void)user_data;
    Wallpaper *wallpaper1 = get_flow_child_wallpaper(child1);
    Wallpaper *wallpaper2 = get_flow_child_wallpaper(child2);
    if (!wallpaper1 || !wallpaper2) return 0;
    gchar *image_path1 = wallpaper1->path;
    gchar *image_path2 = wallpaper2->path;
    return g_strcmp0(image_path1, image_path2);
}

static void widget_block_handler(GtkWidget *widget) {
    gulong *handler = g_object_get_data(G_OBJECT(widget), "handler");
    if (handler) {
        g_signal_handler_block(G_OBJECT(widget), *handler);
    }
}

static void widget_unblock_handler(GtkWidget *widget) {
    gulong *handler = g_object_get_data(G_OBJECT(widget), "handler");
    g_signal_handler_unblock(G_OBJECT(widget), *handler);
}

static void show_images_src_dir(GtkApplication *app) {
    Config *config = g_object_get_data(G_OBJECT(app), "configuration");
    char *source_directory = config->source_directory;
    GtkWidget *flowbox = g_object_get_data(G_OBJECT(app), "flowbox");
    if (!flowbox || is_empty_string(source_directory)) return;

    ArrayWrapper *old_wp_arr_wrapper =
        g_object_get_data(G_OBJECT(app), "wallpapers");
    if (old_wp_arr_wrapper) {
        free_wallpapers(old_wp_arr_wrapper);
        g_object_set_data(G_OBJECT(app), "wallpapers", NULL);
    }

    GtkAdjustment *adjustment = gtk_adjustment_new(0, 0, 100, 1, 10, 0);
    gtk_flow_box_set_vadjustment(GTK_FLOW_BOX(flowbox), adjustment);

    widget_block_handler(flowbox);

    gtk_flow_box_remove_all(GTK_FLOW_BOX(flowbox));

    ArrayWrapper *wp_arr_wrapper = list_wallpapers(source_directory);
    if (!wp_arr_wrapper) return;
    Wallpaper *wallpapers = (Wallpaper *)wp_arr_wrapper->data;

    ArrayWrapper *mon_wrap = g_object_get_data(G_OBJECT(app), "monitors");
    Monitor *monitors = (Monitor *)mon_wrap->data;
    gushort monitor_id;
    if (wallpapers) {
        gtk_flow_box_set_sort_func(GTK_FLOW_BOX(flowbox), NULL, NULL, NULL);
        gushort i;
        for (i = 0; i < wp_arr_wrapper->amount_used; i++) {
            GtkWidget *flow_child = gtk_flow_box_child_new();
            GtkWidget *image = gtk_image_new_from_file(wallpapers[i].path);
            gtk_flow_box_child_set_child(GTK_FLOW_BOX_CHILD(flow_child), image);
            gtk_flow_box_append(GTK_FLOW_BOX(flowbox), flow_child);
            gtk_widget_set_visible(image, true);
            g_object_set_data(G_OBJECT(image), "wallpaper",
                              (gpointer)&wallpapers[i]);
            wallpapers[i].flow_child = GTK_FLOW_BOX_CHILD(flow_child);

            for (monitor_id = 0; monitor_id < mon_wrap->amount_used;
                 monitor_id++) {
                if (monitors[monitor_id].belongs_to_config) {
                    Monitor *monitor = &monitors[monitor_id];
                    ConfigMonitor *bmp =
                        &config->monitors_with_backgrounds[monitor->config_id];
                    if (g_strcmp0(bmp->image_path, wallpapers[i].path) == 0) {
                        monitor->wallpaper = &wallpapers[i];
                    }
                }
            }
        }

        gtk_flow_box_set_sort_func(GTK_FLOW_BOX(flowbox), sort_flow_images,
                                   NULL, NULL);

        g_object_set_data(G_OBJECT(app), "wallpapers",
                          (gpointer)wp_arr_wrapper);
    } else {
        g_print("No images found in %s\n", source_directory);
    }

    widget_unblock_handler(flowbox);
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

    Monitor *monitor = g_object_get_data(G_OBJECT(app), "selected_monitor");

    if (monitor == NULL || bg_mode == BG_MODE_NOT_SET) return;

    if (*menu_choice == DM_BACKGROUND) {
        GtkWidget *flowbox = g_object_get_data(G_OBJECT(app), "flowbox");
        GList *flowbox_children =
            gtk_flow_box_get_selected_children(GTK_FLOW_BOX(flowbox));
        Wallpaper *wallpaper = get_flow_child_wallpaper(flowbox_children->data);
        lightdm_set_background(wallpaper, monitor, bg_mode);
    } else if (monitor->belongs_to_config) {
        ConfigMonitor *config_monitor =
            &config->monitors_with_backgrounds[monitor->config_id];
        config_monitor->bg_mode = bg_mode;

        dump_config(config);
        set_wallpapers(config);
    }
}

static void show_images(GtkButton *button, GtkApplication *app) {
    Config *config = g_object_get_data(G_OBJECT(app), "configuration");
    Monitor *monitor = g_object_get_data(G_OBJECT(button), "monitor");

    GtkLabel *status_label =
        g_object_get_data(G_OBJECT(app), "status_selected_monitor");

    GtkButton *button_menu_choice =
        g_object_get_data(G_OBJECT(app), "menu_choice");

    AppTab *menu_choice =
        g_object_get_data(G_OBJECT(button_menu_choice), "name");

    const gchar *status =
        g_strconcat(gtk_button_get_label(button_menu_choice), " > ",
                    gtk_button_get_label(button), NULL);

    gtk_label_set_label(status_label, status);

    if (monitor) {
        g_object_set_data(G_OBJECT(app), "selected_monitor", (gpointer)monitor);
    } else {
        g_object_set_data(G_OBJECT(app), "selected_monitor", NULL);
    }

    GtkWidget *bg_mode_dropdown =
        g_object_get_data(G_OBJECT(app), "bg_mode_dropdown");
    gtk_widget_set_visible(GTK_WIDGET(bg_mode_dropdown), true);

    BgMode bg_mode = BG_MODE_NOT_SET;

    if (*menu_choice == WM_BACKGROUND && monitor &&
        monitor->belongs_to_config) {
        ConfigMonitor *config_monitor =
            &config->monitors_with_backgrounds[monitor->config_id];
        bg_mode = config_monitor->bg_mode;
    }

    widget_block_handler(bg_mode_dropdown);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(bg_mode_dropdown), bg_mode);
    widget_unblock_handler(bg_mode_dropdown);

    GtkWidget *flowbox;
    flowbox = g_object_get_data(G_OBJECT(app), "flowbox");
    if (flowbox) {
        goto update_selection;
    }

    GtkWidget *vbox = g_object_get_data(G_OBJECT(app), "vbox");
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_box_append(GTK_BOX(vbox), scrolled_window);

    flowbox = gtk_flow_box_new();
    gulong *handler = g_malloc(sizeof(gulong));
    *handler = g_signal_connect_data(
        G_OBJECT(flowbox), "selected-children-changed",
        G_CALLBACK(image_selected), (gpointer)app, NULL, G_CONNECT_DEFAULT);
    g_object_set_data(G_OBJECT(flowbox), "handler", (gpointer)handler);

    gtk_widget_add_css_class(GTK_WIDGET(flowbox), "wallpapers_flowbox");
    gtk_widget_set_vexpand(GTK_WIDGET(flowbox), true);
    g_object_set_data(G_OBJECT(app), "flowbox", flowbox);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window),
                                  flowbox);

    gtk_widget_set_visible(scrolled_window, true);
    gtk_widget_set_visible(flowbox, true);
    show_images_src_dir(app);

update_selection:
    widget_block_handler(flowbox);

    if (*menu_choice == WM_BACKGROUND && monitor->wallpaper) {
        GtkFlowBoxChild *flow_child =
            GTK_FLOW_BOX_CHILD(monitor->wallpaper->flow_child);
        gtk_flow_box_select_child(GTK_FLOW_BOX(flowbox), flow_child);
    } else {
        gtk_flow_box_unselect_all(GTK_FLOW_BOX(flowbox));
    }

    widget_unblock_handler(flowbox);
}

static void show_monitors(GtkApplication *app) {
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
    GtkFileDialog *dialog = gtk_file_dialog_new();
    dialog = gtk_file_dialog_new();

    if (!is_empty_string(config->source_directory)) {
        GFile *initial_src_dir = g_file_new_for_path(config->source_directory);
        gtk_file_dialog_set_initial_folder(dialog, initial_src_dir);
    }

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

    GtkWidget *flowbox = g_object_get_data(G_OBJECT(app), "flowbox");
    gulong *flowbox_handler = g_object_get_data(G_OBJECT(flowbox), "handler");
    if (flowbox_handler) {
        g_signal_handler_disconnect(G_OBJECT(flowbox), *flowbox_handler);
        g_free(flowbox_handler);
    }

    GtkWidget *bg_mode_dropdown =
        g_object_steal_data(G_OBJECT(app), "bg_mode_dropdown");

    gulong *bg_mode_handler =
        g_object_get_data(G_OBJECT(bg_mode_dropdown), "handler");
    if (bg_mode_handler) {
        g_signal_handler_disconnect(G_OBJECT(bg_mode_dropdown),
                                    *bg_mode_handler);
        g_free(bg_mode_handler);
    }

    gtk_widget_unparent(bg_mode_dropdown);
    g_object_set_data(G_OBJECT(app), "bg_mode_options", NULL);

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

    gushort monitor_id;
    gushort config_monitor_id;
    gushort config_monitors_len = config->number_of_monitors;
    ConfigMonitor *bmp = config->monitors_with_backgrounds;

    for (monitor_id = 0; monitor_id < mon_arr_wrapper->amount_used;
         monitor_id++) {
        Monitor *monitor = &monitors[monitor_id];

        gchar *button_label = g_strdup_printf("%s - %d x %d", monitor->name,
                                              monitor->width, monitor->height);
        GtkWidget *button = gtk_button_new_with_label(button_label);
        g_free(button_label);

        for (config_monitor_id = 0; config_monitor_id < config_monitors_len;
             config_monitor_id++) {
            if (g_strcmp0(bmp[config_monitor_id].name, monitor->name) == 0) {
                monitor->config_id = config_monitor_id;
                monitor->belongs_to_config = true;
                break;
            }
        }

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

    GtkWidget *button_settings =
        gtk_button_new_with_label("Choose source directory");
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
        (const gchar *const *)g_ptr_array_free(array, false));
    g_object_set_data(G_OBJECT(app), "bg_mode_options", string_list);

    GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(string_list), NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown), 0);

    gulong *handler = g_malloc(sizeof(gulong));
    *handler = g_signal_connect_data(G_OBJECT(dropdown), "notify::selected",
                                     G_CALLBACK(on_option_selected),
                                     (gpointer)app, NULL, G_CONNECT_DEFAULT);
    g_object_set_data(G_OBJECT(dropdown), "handler", (gpointer)handler);

    gtk_widget_set_visible(GTK_WIDGET(dropdown), false);
    gtk_box_append(GTK_BOX(menu_box), dropdown);
    g_object_set_data(G_OBJECT(app), "bg_mode_dropdown", dropdown);

    GtkWidget *status_selected_monitor = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(status_selected_monitor), false);
    gtk_box_append(GTK_BOX(menu_box), status_selected_monitor);
    g_object_set_data(G_OBJECT(app), "status_selected_monitor",
                      status_selected_monitor);

    gtk_widget_set_visible(window, true);

    gchar css[] = ".wallpapers_flowbox image { min-width: 30em; min-height: "
                  "20em; margin: 0.1em; }";
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, css);
    GdkDisplay *display = gtk_widget_get_display(window);
    gtk_style_context_add_provider_for_display(
        display, GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
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
