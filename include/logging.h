#pragma once

typedef enum { INFO = 1, WARN = 2, ERROR = 3 } LogLevel;

extern void logprintf(LogLevel level, const char *message);
