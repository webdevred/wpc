#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOGGING_FORMAT "%-20s %-10s :: %-10s\n"
#define LOGGING_TIME_FORMAT "%Y-%m-%d %H:%M:%S %Z"

static char *get_time() {
    time_t now;
    time(&now);

    struct tm *local_time = localtime(&now);
    char *buf = malloc(strlen(LOGGING_TIME_FORMAT) + 1);

    strftime(buf, sizeof buf, LOGGING_TIME_FORMAT, local_time);
    return buf;
}

extern void logprintf(LogLevel level, const char *message) {
    char *time = get_time();
    FILE *device = level > INFO ? stderr : stdout;
    fprintf(device, LOGGING_FORMAT, time, "INFO", message);
    free(time);
    if (level > WARN) {
        logprintf(level, "fatal error occurred, exiting");
        exit(1);
    }
}
