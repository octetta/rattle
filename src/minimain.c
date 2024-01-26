#include <stdio.h>
#include <string.h>

#include "amy.h"
#include "rma.h"


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
                fprintf(stdout, "usage: amy-example\n");
                fprintf(stdout, "\t[-d sound device id, use -l to list, default, autodetect]\n");
                fprintf(stdout, "\t[-l list all sound devices and exit]\n");
                fprintf(stdout, "\t[-h show this help and exit]\n");
                return 0;
                break;
            case '?': 
                fprintf(stdout, "option needs a value\n"); 
                break; 
        } 
    }

    amy_start(/* cores= */ 1, /* reverb= */ 1, /* chorus= */ 1);
    amy_live_start();
    amy_reset_oscs();
    
    unsigned int mark;
    int code = 1;
    while (code) {
        char input[1024];
        int len = 0;
        //fprintf(stdout, "# ");
        //fflush(stdout);
        if (fgets(input, sizeof(input)-1, stdin) == NULL) break;
        len = strlen(input);
        if (len == 0) continue;
        char *trimmed = input;
        int trimming = 1;
        while (trimming) {
            switch (*trimmed) {
                case ' ':
                case '\n':
                case '\t':
                    trimmed++;
                    break;
                default:
                    trimming = 0;
                    break;
            }
        }
        len = strlen(trimmed);
        trimming = 1;
        char *rtrim = trimmed + len - 1;
        while (trimming) {
            if (rtrim == trimmed) break; 
            switch (*rtrim) {
                case ' ':
                case '\n':
                case '\t':
                    *rtrim = '\0';
                    rtrim--;
                    break;
                default:
                    trimming = 0;
                    break;
            }
        }

        // fprintf(stdout, "<%s>\n", trimmed);

        len = strlen(input);
        if (len == 0) continue;

        switch (*trimmed) {
            case '?':
                switch (*(trimmed+1)) {
                    case 'c':
                        fprintf(stdout, "%d\n", amy_sysclock());
                        break;
                }
                break;
            default:
                amy_play_message(trimmed);
                break;
        }


        code = 1;
    }

    amy_live_stop();
    
    return 0;
}
