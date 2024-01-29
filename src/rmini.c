//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//#include "amy.h"
//#include "rma.h"

#define SLEN (44100)

#include "ratlib.h"

int main(int argc, char *argv[]) {
    int opt;
    short int *sbuf = NULL;
    unsigned int slen = SLEN;
    short int nchan = 2;
    while ((opt = getopt(argc, argv, ":d:lh")) != -1) {
        switch(opt) { 
            case 'd': 
                rat_device(atoi(optarg));
                break;
            case 'l':
                rat_list();
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

    rat_start();

    int code = 1;

    while (code) {
        char input[1024];
        int len = 0;
        //fprintf(stdout, "# ");
        fflush(stdout);
        if (fgets(input, sizeof(input)-1, stdin) == NULL) break;
        
        len = strlen(input);
        if (len == 0) continue;

        // scan for comments and semicolons

        char *trimmed = rat_scan(input);

        //fprintf(stderr, "<%s>\n", trimmed);

        switch (*trimmed) {
            case '?': // query
                switch (*(trimmed+1)) {
                    case 'c':
                        fprintf(stdout, "%d\n", rat_clock());
                        break;
                    case 'i':
                        fprintf(stdout, "[%d,%d]\n", rat_frames(), nchan);
                        break;
                    case 'n':
                        {
                            int n = rat_frames();
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
                            rat_frame_stop();
                            rat_frame_start(sbuf, slen, &nchan);
                        }
                    } else {
                        rat_frame_stop();
                    }
                }
                break;
            default:
                rat_send(trimmed);
                break;
        }


        code = 1;
    }

    rat_stop();
    
    return 0;
}
