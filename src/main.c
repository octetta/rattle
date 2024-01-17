#include <stdio.h>
#include <sys/select.h>

#include "amy.h"
#include "rma.h"
#include "rattlefy.h"
#include "linenoise.h"

static int use_linenoise = 1;

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, ":d:lhR")) != -1) { 
        switch(opt) { 
            case 'R': 
                use_linenoise = 0;
                break;
            case 'd': 
                amy_device_id = atoi(optarg);
                break;
            case 'l':
                amy_print_devices();
                return 0;
                break;
            case 'h':
                INFO("usage: amy-example\n");
                INFO("\t[-d sound device id, use -l to list, default, autodetect]\n");
                INFO("\t[-l list all sound devices and exit]\n");
                INFO("\t[-h show this help and exit]\n");
                return 0;
                break;
            case SETTING: 
                INFO("option needs a value\n"); 
                break; 
            case QUERY: 
                INFO("unknown option: %c\n", optopt);
                break; 
        } 
    }
    amy_start(/* cores= */ 1, /* reverb= */ 0, /* chorus= */ 0);
    amy_live_start();
    amy_reset_oscs();
    int code = 1;
    if (use_linenoise) {
        linenoiseHistorySetMaxLen(999);
        linenoiseHistoryLoad(".rattle_history");
    }
    while (code) {
        char input[1024];
        if (use_linenoise) {
#if 0
            char *line = linenoise(PROMPT);
            if (line == NULL) break;
            strcpy(input, line);
            linenoiseHistoryAdd(line);
            linenoiseFree(line);
#else
            char *line = NULL;
            struct linenoiseState ls;
            char buf[1024];
            linenoiseEditStart(&ls, -1, -1, buf, sizeof(buf), PROMPT);
            while (1) {
                fd_set readfds;
                struct timeval tv;
                int r;
                FD_ZERO(&readfds);
                FD_SET(ls.ifd, &readfds);
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                r = select(ls.ifd+1, &readfds, NULL, NULL, &tv);
                if (r == -1) {
                    printf("select failed\n");
                    code = 0;
                    break;
                } else if (r) {
                    line = linenoiseEditFeed(&ls);
                    if (line != linenoiseEditMore) break;
                } else {
                    // timeout
                    static int counter = 0;
                    linenoiseHide(&ls);
                    printf("count %d\n", counter++);
                    linenoiseShow(&ls);
                }
            }
            linenoiseEditStop(&ls);
            if (line == NULL) break;
#endif
        } else {
            INFO(PROMPT);
            fflush(stdout);
            if (fgets(input, sizeof(input)/2, stdin) == NULL) break;
        }
        char *prior = strdup(input);
        char *token = strtok(input, splitter);

        unsigned int now = amy_sysclock();

        while (token != NULL) {
            int n = token - input - 1;
            char c = 0;
            if (n > 0) c = prior[n];
            code = process(now, token, c);
            token = strtok(NULL, splitter);
        }
        free(prior);
    }
    amy_live_stop();
    if (use_linenoise) {
        linenoiseHistorySave(".rattle_history");
    }
    return 0;
}
