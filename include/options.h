#pragma once

typedef enum {
    DAEMON_SET_BACKGROUNDS,
    SET_BACKGROUNDS_AND_EXIT,
    START_GUI
} Action;

typedef struct {
    Action action;
} Options;

void parse_options(char **argv, Options *options);
