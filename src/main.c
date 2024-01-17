#include <stdio.h>

#include "amy.h"
#include "rma.h"
#include "rattlefy.h"

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, ":d:lh")) != -1) { 
        switch(opt) { 
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
    while (code) {
        char input[1024];
        INFO(PROMPT);
        fflush(stdout);
        //if (fgets(input, sizeof(input)/2, stdin) == NULL) break;
        if (repl(input, sizeof(input)/2, stdin) == NULL) break;

        char *prior = strdup(input);
        char *token = strtok(input, splitter);

        unsigned int now = amy_sysclock();

        //VERBOSE("NOW=%d\n", now);

        while (token != NULL) {
            int n = token - input - 1;
            char c = 0;
            if (n > 0) c = prior[n];
            //VERBOSE("[%d]\n", ntok++);
            code = process(now, token, c);
            token = strtok(NULL, splitter);
        }
        free(prior);
    }
    amy_live_stop();
    return 0;
}
