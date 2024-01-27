#include <stdio.h>
#include <string.h>

#include "amy.h"
#include "rma.h"

#define SLEN (44100)

int main(int argc, char *argv[]) {
    int opt;
    short int *sbuf = NULL;
    unsigned int slen = SLEN;
    short int nchan = 2;
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
        fflush(stdout);
        if (fgets(input, sizeof(input)-1, stdin) == NULL) break;
        // scan for comments and semicolons
        len = strlen(input);
        if (len == 0) continue;
        char *first = input;
        int scanning = 1;
        while (scanning) {
            switch (*first) {
                case '#':
                case ';':
                    *first = '\0';
                case '\0':
                    scanning = 0;
                    break;
            }
            first++;
        }
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

        fprintf(stderr, "<%s>\n", trimmed);

        len = strlen(input);
        if (len == 0) continue;

        switch (*trimmed) {
            case '?': // query
                switch (*(trimmed+1)) {
                    case 'c':
                        fprintf(stdout, "%d\n", amy_sysclock());
                        break;
                    case 'i':
                        fprintf(stdout, "[%d,%d]\n", captured_frames(), nchan);
                        break;
                    case 'n':
                        {
                            int n = captured_frames();
                            fprintf(stdout, "[");
                            char c = ',';
                            for (int i=0; i<n; i++) {
                                if (i == n-1) c = ']';
                                printf("%d%c", sbuf[i], c);
                            }
                            fprintf(stdout, "\n");
                        }
                        break;
                }
                break;
            case '<': // sample
                {
                    char *num = trimmed+1;
                    if (*num == '\0') {
                        slen = SLEN;
                    } else {
                        slen = strtol(num, NULL, 10);
                    }
                    if (slen > 0) {
                        if (sbuf) free(sbuf);
                        sbuf = (short int *)malloc(slen * sizeof(short int));
                        if (sbuf) {
                            capture_stop();
                            capture_start(sbuf, slen, &nchan);
                        }
                    } else {
                        capture_stop();
                    }
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
