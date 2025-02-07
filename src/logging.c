#include "logging.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOGGING_FORMAT "%-20s %-10s :: %-10s\n"
#define LOGGING_TIME_FORMAT "%Y-%m-%d %H:%M:%S %Z"

static char *get_time() {
    time_t now;
    time(&now);

    struct tm *local_time = localtime(&now);
    char buf[sizeof LOGGING_TIME_FORMAT];

    strftime(buf, sizeof buf, LOGGING_TIME_FORMAT, local_time);
    return strdup(buf);
}

extern void logprintf(LogLevel level, char *message) {
  char *time = get_time();
  FILE *device = level > INFO ? stderr : stdout;
  fprintf(device,LOGGING_FORMAT, time, "INFO", message);
  free(time);
  if (level > WARN) {
      logprintf(level, "fatal error occurred, exiting");
      exit(1);
  }
}
