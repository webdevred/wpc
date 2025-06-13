#define NOB_IMPLEMENTATION

#include "nob.h"

#include <ctype.h>
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
    nob_cmd_append(cmd, "-std=gnu2x");
    if (use_imagemagick7) nob_cmd_append(cmd, "-DWPC_IMAGEMAGICK_7=1");
    if (enable_lightdm_helper) {
        nob_cmd_append(
            cmd, "-DWPC_ENABLE_HELPER=1",
            nob_temp_sprintf("-DWPC_HELPER_PATH=\"%s\"", lightdm_helper_path));
    }

    nob_cc_flags(cmd);
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

char *run_cmd_and_get_output(Nob_Cmd *cmd) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return NULL;
    }

    Nob_Cmd_Redirect cmd_red = {0};
    cmd_red.fdout = &pipefd[1];
    nob_cmd_run_async_redirect(*cmd, cmd_red);
    cmd->count = 0;

    close(pipefd[1]);

    char *buffer = NULL;
    size_t total_size = 0;
    char tmp_buffer[256];

    ssize_t bytes_read;
    while ((bytes_read = read(pipefd[0], tmp_buffer, sizeof(tmp_buffer))) > 0) {
        char *new_buffer = realloc(buffer, total_size + bytes_read + 1);
        if (!new_buffer) {
            free(buffer);
            close(pipefd[0]);
            return NULL;
        }
        buffer = new_buffer;
        memcpy(buffer + total_size, tmp_buffer, bytes_read);
        total_size += bytes_read;
    }

    if (buffer) {
        buffer[total_size] = '\0';
    }

    close(pipefd[0]);
    return buffer;
}

LibFlagsDa list_lib_flags(Nob_Cmd *cmd, char *lib_names[], bool cflags) {
    if (cflags) {
        nob_cmd_append(cmd, "pkg-config", "--cflags");
    } else {
        nob_cmd_append(cmd, "pkg-config", "--libs");
    }

    char **lib_name = lib_names;
    while (*lib_name) {
        nob_cmd_append(cmd, *lib_name);
        lib_name++;
    }

    char *buffer = run_cmd_and_get_output(cmd);

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
                if (strcmp(object, "common.o") == 0 ||
                    strcmp(object, "wpc_lightdm_helper.o") == 0) {
                    nob_da_append(&objects, object_place);
                } else {
                    continue;
                }
            }

            nob_cmd_append(cmd, "clang");

            if (enable_dev_tooling) {
                ext--;
                *ext = '\0';
                nob_cmd_append(
                    cmd, "-MJ",
                    nob_temp_sprintf("%s/%s.json", BUILD_FOLDER, object));
            }

            build_object(cmd, main_cflags, common_cflags);

            nob_cmd_append(cmd, "-c");

            if (!ext) continue;

            nob_cc_inputs(
                cmd, nob_temp_sprintf("%s/%s", SRC_FOLDER, files.items[i]));
            nob_cc_output(cmd, object_place);
            if (!nob_cmd_run_sync_and_reset(cmd)) exit(1);
            cmd->count = 0;
            free(object);
        }
    }

    return objects;
}

int build_target(Nob_Cmd *cmd, const char *target, Nob_File_Paths objects,
                 LibFlagsDa *main_ldflags, LibFlagsDa *common_ldflags) {
    nob_cmd_append(cmd, "clang");
    const char *out_file = nob_temp_sprintf("%s/%s", BUILD_FOLDER, target);
    uint nobjects = objects.count;
    for (uint i = 0; i < nobjects; i++) {
        nob_cmd_append(cmd, objects.items[i]);
    }
    build_object(cmd, main_ldflags, common_ldflags);

    nob_cc_output(cmd, out_file);

    if (!nob_cmd_run_sync_and_reset(cmd)) return 1;
    cmd->count = 0;
    return 0;
}
void should_use_imagemagick7(Nob_Cmd *cmd) {
    char *imagemagick_version_env = getenv("WPC_IMAGEMAGICK_7");
    if (imagemagick_version_env != NULL) {
        use_imagemagick7 = strcmp(imagemagick_version_env, "0") != 0;
        goto log;
    }

    nob_log(NOB_INFO, "No environment override found. Detecting ImageMagick "
                      "version using pkg-config...");
    nob_cmd_append(cmd, "pkg-config", "--modversion", "MagickWand");

    char *buffer = run_cmd_and_get_output(cmd);

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
    char imagemagick_version = use_imagemagick7 ? '7' : '6';
    nob_log(NOB_INFO,
            "Chosen ImageMagick version: %c based on environment or system "
            "detection",
            imagemagick_version);
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

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    Nob_Cmd cmd = {0};
    check_dev_tooling_enabled();
    should_use_imagemagick7(&cmd);
    setup_lightdm_helper_flags();

    char *wpc_libs[] = {"gtk4",       "glib-2.0", "x11", "xrandr",
                        "MagickWand", "libmagic", NULL};
    char *wpc_common_libs[] = {"libcjson", NULL};

    LibFlagsDa wpc_common_cflags = list_lib_cflags(&cmd, wpc_common_libs);
    LibFlagsDa wpc_common_ldflags = list_lib_ldflags(&cmd, wpc_common_libs);

    LibFlagsDa wpc_cflags = list_lib_cflags(&cmd, wpc_libs);
    LibFlagsDa wpc_ldflags = list_lib_ldflags(&cmd, wpc_libs);

    LibFlagsDa wpc_helper_cflags = {0};
    LibFlagsDa wpc_helper_ldflags = {0};

    Nob_File_Paths object_names =
        build_source_files(&cmd, "wpc", &wpc_cflags, &wpc_common_cflags);

    build_target(&cmd, "wpc", object_names, &wpc_ldflags, &wpc_common_ldflags);

    Nob_File_Paths helper_objects = build_source_files(
        &cmd, "wpc_lightdm_helper", &wpc_helper_cflags, &wpc_common_cflags);

    build_target(&cmd, "wpc_lightdm_helper", helper_objects,
                 &wpc_helper_ldflags, &wpc_common_ldflags);

    nob_da_free(wpc_cflags);
    nob_da_free(wpc_ldflags);
    nob_da_free(wpc_common_cflags);
    nob_da_free(wpc_common_ldflags);

    return 0;
}
