#include <stdio.h>
#include <string.h>

#include "amy.h"
#include "rma.h"

#define SLEN (44100)

void rat_device(int d) {
    amy_device_id = d;
}

void rat_list(void) {
    amy_print_devices();
}

char *rat_scan(char *input) {
    int len = strlen(input);
    if (len == 0) return input;
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
    return trimmed;
}

void rat_start(void) {
    amy_start(/* cores= */ 1, /* reverb= */ 1, /* chorus= */ 1);
    amy_live_start();
    amy_reset_oscs();
}

void rat_send(char *trimmed) {
    amy_play_message(trimmed);
}

void rat_stop(void) {
    amy_live_stop();
}

int rat_frames(void) {
    return captured_frames();
}

void rat_frame_stop(void) {
    capture_stop();
}

void rat_frame_start(short int *sbuf, int slen, short int *n) {
    capture_start(sbuf, slen, n);
}

unsigned int rat_clock(void) {
    return amy_sysclock();
}
