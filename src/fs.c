#include <dirent.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "fs.h"

extern char **list_images(char *source_directory, int *number_of_images) {
    char **filenamesArray = NULL;
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
                                     (filenameArraySize + 1) * sizeof(char *));
            if (!filenamesArray) {
                closedir(dir);
            }

            filenamesArray[filenameArraySize] =
                malloc(strlen(source_directory) * sizeof(char *) +
                       strlen(filename) * sizeof(char *) + sizeof(char *));
            if (filenamesArray[filenameArraySize]) {
                char **image_path_ptr = &filenamesArray[filenameArraySize];
                strcpy(*image_path_ptr, source_directory);
                strcat(*image_path_ptr, filename);
                filenameArraySize++;
            }
        }
        closedir(dir);

        filenamesArray =
            realloc(filenamesArray, (filenameArraySize + 1) * sizeof(char *));
        filenamesArray[filenameArraySize] = NULL;
    }

    *number_of_images = filenameArraySize;

    return filenamesArray;
}
