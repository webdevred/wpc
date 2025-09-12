#include <unistd.h>
#include <utime.h>
#define NOB_WARN_DEPRECATED
#define NOB_IMPLEMENTATION

#if defined(__clang__)
#define NOB_REBUILD_URSELF(binary_path, source_path)                           \
    "clang", "-MJ", "build/nob.o.json", "-o", binary_path, source_path
#else
#define NOB_REBUILD_URSELF(binary_path, source_path)                           \
    "cc", "-o", binary_path, source_path
#endif

#include "nob.h"
// Some folder paths that we use throughout the build process.
#define BUILD_FOLDER "build"
#define SRC_FOLDER "src"
#define HEADER_FOLDER "include"

typedef struct {
    uint count;
    uint capacity;
    char **items;
} LibFlagsDa;

bool use_imagemagick7 = false;
bool enable_lightdm_helper = true;
bool enable_dev_tooling = false;
char lightdm_helper_path[256];

void build_object(Nob_Cmd *cmd, LibFlagsDa *main_flags,
                  LibFlagsDa *common_flags) {
    char *lib = nob_temp_sprintf("-I%s", HEADER_FOLDER);
    nob_cmd_append(cmd, "-std=c11");
    if (use_imagemagick7) nob_cmd_append(cmd, "-DWPC_IMAGEMAGICK_7=1");
    if (enable_lightdm_helper) {
        nob_cmd_append(
            cmd, "-DWPC_ENABLE_HELPER=1",
            nob_temp_sprintf("-DWPC_HELPER_PATH=\"%s\"", lightdm_helper_path));
    }

    if (enable_dev_tooling) {
        nob_cmd_append(cmd, "-g", "-fsanitize=address",
                       "-fno-omit-frame-pointer");
    }
    nob_cmd_append(cmd, "-std=c11", "-Werror", "-Wpedantic",
                   "-Wdeclaration-after-statement", "-Wmissing-prototypes",
                   "-Wstrict-prototypes", "-Wmissing-variable-declarations",
                   "-Wshadow", "-Wformat=2", "-Wundef", "-Wformat-truncation",
                   "-Wconversion", "-Wuninitialized", "-Wnested-externs",
                   "-Wunused-function", "-Wunused-variable",
                   "-Wdouble-promotion");

    nob_cmd_append(cmd, lib);
    uint nmain_flags = main_flags->count;
    uint ncommon_flags = common_flags->count;
    uint i;
    for (i = 0; i < nmain_flags; i++) {

        nob_cmd_append(cmd, main_flags->items[i]);
    }
    for (i = 0; i < ncommon_flags; i++) {
        nob_cmd_append(cmd, common_flags->items[i]);
    }
}

char *run_cmd_and_get_output(Nob_Cmd *cmd, char *tmp_filename) {
    char template[] = "/tmp/tmpdir.wpc-nob-XXXXXX";
    char *dir_name = mkdtemp(template);
    char *file_path = nob_temp_sprintf("%s/%s.txt", dir_name, tmp_filename);
    Nob_String_Builder builder = {0};

    nob_cmd_run(cmd, .stdout_path = file_path);

    nob_read_entire_file(file_path, &builder);
    char *output = strdup(builder.items);
    unlink(file_path);
    rmdir(dir_name);
    nob_da_free(builder);
    return output;
}

LibFlagsDa list_lib_flags(Nob_Cmd *cmd, char *lib_names[], bool cflags) {
    char *name;
    if (cflags) {
        name = strdup("lib_cflags");
        nob_cmd_append(cmd, "pkg-config", "--cflags");
    } else {
        name = strdup("lib_flags");
        nob_cmd_append(cmd, "pkg-config", "--libs");
    }

    char **lib_name = lib_names;
    while (*lib_name) {
        nob_cmd_append(cmd, *lib_name);
        lib_name++;
    }

    char *buffer = run_cmd_and_get_output(cmd, name);

    LibFlagsDa lib_places = {0};
    char *lib = strtok(buffer, " ");
    nob_da_append(&lib_places, strdup(lib));
    while ((lib = strtok(NULL, " ")) != NULL) {
        if (isspace(lib[0])) {
            continue;
        }
        nob_da_append(&lib_places, strdup(lib));
        lib += strlen(lib) + 1;
    }
    free(name);
    free(buffer);
    cmd->count = 0;
    return lib_places;
}

LibFlagsDa list_lib_cflags(Nob_Cmd *cmd, char *lib_names[]) {
    return list_lib_flags(cmd, lib_names, true);
}

LibFlagsDa list_lib_ldflags(Nob_Cmd *cmd, char *lib_names[]) {
    return list_lib_flags(cmd, lib_names, false);
}

void write_json_from_builder(Nob_String_Builder *builder, FILE *comp_db,
                             bool first) {
    if (!first) fwrite(",", sizeof(char), 1, comp_db);
    fwrite("\n  ", sizeof(char), 3, comp_db);
    fwrite(builder->items, sizeof(char), builder->count - 2, comp_db);
}

void create_compilation_database(Nob_File_Paths files) {
    FILE *comp_db = fopen("compile_commands.json", "w");
    uint i;
    Nob_String_Builder builder = {0};
    char *json_path;
    bool nob_json_read_success = false;
    fwrite("[", sizeof(char), 1, comp_db);
    for (i = 0; i < files.count; i++) {
        builder.count = 0;
        json_path = nob_temp_sprintf("%s.json", files.items[i]);
        nob_read_entire_file(json_path, &builder);
        nob_log(NOB_INFO, "adding %s to compilation database", json_path);
        write_json_from_builder(&builder, comp_db, i == 0);
    }

    builder.count = 0;
    nob_json_read_success = nob_read_entire_file("build/nob.o.json", &builder);
    if (nob_json_read_success && i > 0) {
        nob_log(NOB_INFO, "adding nob.o.json to compilation database");
        write_json_from_builder(&builder, comp_db, false);
    }

    fwrite("\n]\n", sizeof(char), 3, comp_db);
}

Nob_File_Paths build_source_files(Nob_Cmd *cmd, const char *target,
                                  LibFlagsDa *main_cflags,
                                  LibFlagsDa *common_cflags) {
    Nob_File_Paths files = {0};
    nob_read_entire_dir(SRC_FOLDER, &files);
    uint i;
    uint nfiles = files.count;

    Nob_File_Paths objects = {0};
    for (i = 0; i < nfiles; i++) {
        const char *filename = files.items[i];
        uint file_path_len = strlen(files.items[i]);
        if (strncmp(filename, "#", 1) != 0 && strncmp(filename, ".", 1) != 0 &&
            filename[file_path_len - 1] != '~') {
            char *object = strdup((char *)files.items[i]);
            char *ext = strrchr(object, '.');
            ext++;
            *ext = 'o';
            char *object_place =
                nob_temp_sprintf("%s/%s", BUILD_FOLDER, object);
            if (strcmp(target, "wpc") == 0) {
                if (strcmp(object, "wpc_lightdm_helper.o") != 0) {
                    if (strcmp(object, "lightdm.o") == 0 &&
                        !enable_lightdm_helper)
                        continue;
                    nob_da_append(&objects, object_place);
                } else {
                    continue;
                }
            } else if (strcmp(target, "wpc_lightdm_helper") == 0) {
                if (strcmp(object, "wpc_lightdm_helper.o") == 0 ||
                    strcmp(object, "common.o") == 0) {
                    nob_da_append(&objects, object_place);
                } else {
                    continue;
                }
            }

            if (enable_dev_tooling) {
                char *compilation_json_file =
                    nob_temp_sprintf("%s/%s.json", BUILD_FOLDER, object);
                nob_cmd_append(cmd, "clang");
                ext--;
                *ext = '\0';
                nob_cmd_append(cmd, "-MJ", compilation_json_file);
            } else {
                nob_cc(cmd);
            }

            build_object(cmd, main_cflags, common_cflags);

            nob_cmd_append(cmd, "-c");

            if (!ext) continue;

            nob_cc_inputs(
                cmd, nob_temp_sprintf("%s/%s", SRC_FOLDER, files.items[i]));
            nob_cc_output(cmd, object_place);
            if (!nob_cmd_run(cmd)) exit(1);
            free(object);
        }
    }

    return objects;
}

int build_target(Nob_Cmd *cmd, const char *target, Nob_File_Paths objects,
                 LibFlagsDa *main_ldflags, LibFlagsDa *common_ldflags) {
    if (enable_dev_tooling) {
        nob_cmd_append(cmd, "clang");
    } else {
        nob_cc(cmd);
    }
    const char *out_file = nob_temp_sprintf("%s/%s", BUILD_FOLDER, target);
    uint nobjects = objects.count;
    for (uint i = 0; i < nobjects; i++) {
        nob_cmd_append(cmd, objects.items[i]);
    }
    build_object(cmd, main_ldflags, common_ldflags);

    nob_cc_output(cmd, out_file);

    if (!nob_cmd_run(cmd)) return 1;
    cmd->count = 0;
    return 0;
}
void should_use_imagemagick7(Nob_Cmd *cmd) {
    char *imagemagick_version_env = getenv("WPC_IMAGEMAGICK_7");
    if (imagemagick_version_env != NULL) {
        use_imagemagick7 = strcmp(imagemagick_version_env, "1") == 0;
        goto log;
    }

    nob_log(NOB_INFO, "No environment override found. Detecting ImageMagick "
                      "version using pkg-config...");
    nob_cmd_append(cmd, "pkg-config", "--modversion", "MagickWand");

    char *buffer = run_cmd_and_get_output(cmd, "imagemagick");

    if (strncmp(buffer, "7.", 2) == 0) {
        use_imagemagick7 = true;
    } else if (strncmp(buffer, "6.", 2) == 0) {
        use_imagemagick7 = false;
    } else {
        nob_log(NOB_ERROR, "Couldnt find good ImageMagick. Only versions 6 and "
                           "7 are supported.");
        exit(1);
    }
    free(buffer);
    cmd->count = 0;
log:
    nob_log(NOB_INFO,
            "Chosen ImageMagick version: %c based on environment or system "
            "detection",
            use_imagemagick7 ? '7' : '6');
    return;
}

void setup_lightdm_helper_flags(void) {
    char *enable_helper_var = getenv("WPC_HELPER");
    char *helper_path_var = getenv("WPC_HELPER_PATH");

    if (helper_path_var != NULL) {
        strcpy(lightdm_helper_path, helper_path_var);
    } else {
        strcpy(lightdm_helper_path, "/usr/local/libexec/wpc/lightdm_helper");
    }

    if (enable_helper_var != NULL) {
        enable_lightdm_helper = strcmp(enable_helper_var, "0") != 0;
    }

    if (enable_lightdm_helper) {
        nob_log(NOB_INFO, "LightDM helper enabled. Executable path: %s",
                lightdm_helper_path);
    } else {
        nob_log(NOB_INFO, "LightDM helper disabled by configuration");
    }
}

void check_dev_tooling_enabled(void) {
    char *enable_dev_tooling_var = getenv("WPC_DEV");

    if (enable_dev_tooling_var != NULL) {
        enable_dev_tooling = strcmp(enable_dev_tooling_var, "0") != 0;
    }

    nob_log(NOB_INFO, "Decided to build in %s mode.",
            enable_dev_tooling ? "dev" : "prod");
}

bool nob_touch_file(const char *path) {
    nob_log(NOB_INFO, "touching %s", path);
    if (utime(path, NULL) == -1) {
        if (errno != ENOENT) {
            nob_log(NOB_ERROR, "Could not touch file %s: %s", path,
                    strerror(errno));
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv) {
    check_dev_tooling_enabled();
#if defined(__clang__)
    if (!nob_file_exists("build/nob.o.json") && enable_dev_tooling) {
        nob_touch_file("nob.c");
    }
#endif
    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_Cmd cmd = {0};
    should_use_imagemagick7(&cmd);
    setup_lightdm_helper_flags();

    char *wpc_libs[] = {"gtk4",       "x11",      "xrandr",
                        "MagickWand", "libmagic", NULL};
    char *wpc_common_libs[] = {"glib-2.0", "libcjson", NULL};

    LibFlagsDa wpc_common_cflags = list_lib_cflags(&cmd, wpc_common_libs);
    LibFlagsDa wpc_common_ldflags = list_lib_ldflags(&cmd, wpc_common_libs);

    LibFlagsDa wpc_cflags = list_lib_cflags(&cmd, wpc_libs);
    LibFlagsDa wpc_ldflags = list_lib_ldflags(&cmd, wpc_libs);

    LibFlagsDa wpc_helper_cflags = {0};
    LibFlagsDa wpc_helper_ldflags = {0};

    Nob_File_Paths object_names =
        build_source_files(&cmd, "wpc", &wpc_cflags, &wpc_common_cflags);

    build_target(&cmd, "wpc", object_names, &wpc_ldflags, &wpc_common_ldflags);

    if (enable_lightdm_helper) {
        Nob_File_Paths helper_objects = build_source_files(
            &cmd, "wpc_lightdm_helper", &wpc_helper_cflags, &wpc_common_cflags);

        build_target(&cmd, "wpc_lightdm_helper", helper_objects,
                     &wpc_helper_ldflags, &wpc_common_ldflags);
    }
#if defined(__clang__)
    if (enable_dev_tooling) {
        create_compilation_database(object_names);
    }
#endif
    nob_da_free(wpc_cflags);
    nob_da_free(wpc_ldflags);
    nob_da_free(wpc_common_cflags);
    nob_da_free(wpc_common_ldflags);

    return 0;
}
