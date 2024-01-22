#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#if 0
#include <sys/select.h>
#else
#include <poll.h>
#endif

#include "amy.h"
#include "rma.h"
#include "rattlefy.h"
#include "linenoise.h"

static int use_linenoise = 1;
// multiplexer between linenoise and looper... needs a fallback for fgets?

int serve_fd = -1;

void poll_ms(int ms) {
    while (1) {
        int r;
        struct pollfd p[1];
        p[0].fd = serve_fd;
        p[0].events = POLLIN;
        r = poll(p, 1, ms);
        if (r == -1) {
            break;
        } else if (r) {
            if (p[0].revents & POLLIN) {
                char dump[1025];
                int n = read(p[0].fd, dump, sizeof(dump)-1);
                if (n > 0) {
                    dump[n] = '\0';
                    unsigned int mark = amy_sysclock();
                    process(mark, dump);
                }
            }
        } else {
            // timeout
            break;
        }
    }
}

int multiplex(char *input) {
    char *line = NULL;
    struct linenoiseState ls;
    char buf[1024];
    linenoiseEditStart(&ls, -1, -1, buf, sizeof(buf), PROMPT);
    while (1) {
        int r;
#if 0
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(ls.ifd, &readfds);
        int maxfd = ls.ifd;
        r = select(maxfd+1, &readfds, NULL, NULL, &tv);
#else
        struct pollfd p[2];
        p[0].fd = ls.ifd;
        p[0].events = POLLIN;
        p[1].fd = serve_fd;
        p[1].events = POLLIN;
        r = poll(p, 2, 1000);
#endif
        if (r == -1) {
            printf("multiplex failed\n");
            //code = 0;
            break;
        } else if (r) {
#if 0
#else
            if (p[0].revents & POLLIN) {
                line = linenoiseEditFeed(&ls);
                if (line != linenoiseEditMore) break;
            }
            if (p[1].revents & POLLIN) {
                char dump[1025];
                int n = read(p[1].fd, dump, sizeof(dump)-1);
                if (n > 0) {
                    dump[n] = '\0';
                    unsigned int mark = amy_sysclock();
                    process(mark, dump);
                }
            }
#endif
        } else {
            // timeout
        }
    }
    linenoiseEditStop(&ls);
    if (line == NULL) return -1;
    int len = strlen(line);
    if (len > 0) {
        strcpy(input, line);
        linenoiseHistoryAdd(line);
        return len;
    }
    linenoiseFree(line);
    return 0;
}

int udp_port = 8405;

int main(int argc, char *argv[]) {
    int opt;
    char *use_file = NULL;
    while ((opt = getopt(argc, argv, ":d:lhRf:u:")) != -1) {
        switch(opt) { 
            case 'u':
                udp_port = atoi(optarg);
                break;
            case 'f':
                use_file = optarg;
                break;
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

    serve_fd = udp_open(udp_port);
    if (serve_fd < 0) {
        printf("can't open udp port %d\n", udp_port);
        perror("udp_open");
        return 1;
    }
    
    raw_setter(SYS, 'm', "250");
    init_looper();

    amy_start(/* cores= */ 1, /* reverb= */ 1, /* chorus= */ 1);
    amy_live_start();
    amy_reset_oscs();
    
    motor_init(2);

    int code = 1;
    
    set_loader_ms(poll_ms);

    if (use_file) {
        loader(use_file);
    }
    
    if (use_linenoise) {
        linenoiseHistorySetMaxLen(999);
        linenoiseHistoryLoad(".rattle_history");
    }
    
    unsigned int mark;
    while (code) {
        char input[1024];
        int len = 0;
        if (use_linenoise) {
            len = multiplex(input);
            if (len < 0) break;
        } else {
            INFO(PROMPT);
            fflush(stdout);
            if (fgets(input, sizeof(input)/2, stdin) == NULL) break;
            len = strlen(input);
        }
        
        mark = amy_sysclock();
        
        printf("interval:%d/%f loop:%d/%f fc:%d BS:%d NC:%d SR:%d\n",
            interval_amy,
            (double)interval_os.tv_usec/1000.0,
            loop_amy,
            (double)loop_os.tv_usec/1000.0,
            cb_frame_count,
            AMY_BLOCK_SIZE, AMY_NCHANS, AMY_SAMPLE_RATE);

        if (len == 0) continue;
        
        code = process(mark, input);

    }

    amy_live_stop();
    
    if (use_linenoise) {
        linenoiseHistorySave(".rattle_history");
    }
    
    puts("rattle done");
    
    return 0;
}
