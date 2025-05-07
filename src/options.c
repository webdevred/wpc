#include "options.h"

void parse_options(char **argv, Options *options) {
    argv++;
    *options = (Options){.action = START_GUI};
    if (!*argv) {
        return;
    }

    while (**argv) {
        switch (**argv) {
        case 'b':
            options->action = SET_BACKGROUNDS_AND_EXIT;
            break;
        case 'd':
            options->action = DAEMON_SET_BACKGROUNDS;
            break;
        }
        (*argv)++;
    }
}
