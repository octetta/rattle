#include <stdio.h>
#if 0
#include <sys/select.h>
#else
#include <poll.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>


#include "amy.h"
#include "rma.h"
#include "rattlefy.h"
#include "linenoise.h"
#include "tinycthread.h"

static int use_linenoise = 1;

static struct sockaddr_in serve;

int udp_open(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
    bzero(&serve, sizeof(serve)); 
    serve.sin_family = AF_INET;
    serve.sin_addr.s_addr = htonl(INADDR_ANY);
    serve.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&serve, sizeof(serve)) >= 0) {
        return sock;
    }
    return -1;
}

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
    int serve_fd = udp_open(8405);
    int send_fd = udp_open(8406);

    printf("serve_fd = %d\n", serve_fd);
    printf("send_fd = %d\n", send_fd);

    set_signal_fd(send_fd);

    amy_start(/* cores= */ 1, /* reverb= */ 0, /* chorus= */ 0);
    amy_live_start();
    amy_reset_oscs();
    int code = 1;
    if (use_linenoise) {
        linenoiseHistorySetMaxLen(999);
        linenoiseHistoryLoad(".rattle_history");
    }
    
    //thrd_t t;
    //if (thrd_create(&t, metro, (void *)0) == thrd_success) {
    //    printf("METRO WORKED\n");
    //}
    
    unsigned int mark;
    while (code) {
        char input[1024];
        int len = 0;
        if (use_linenoise) {
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
                r = select(ls.ifd+1, &readfds, NULL, NULL, &tv);
#else
                struct pollfd p[2];
                p[0].fd = ls.ifd;
                p[0].events = POLLIN;
                p[1].fd = serve_fd;
                p[1].events = POLLIN;
                r = poll(p, 2, 1000);
#endif
                if (r == -1) {
                    printf("select failed\n");
                    code = 0;
                    break;
                } else if (r) {
                    if (p[0].revents & POLLIN) {
                        line = linenoiseEditFeed(&ls);
                        if (line != linenoiseEditMore) break;
                    }
                    if (p[1].revents & POLLIN) {
                        char buf[1024];
                        read(serve_fd, buf, 1024);
                        puts("!");
                    }
                } else {
                    // timeout
#if 0
                    static int counter = 0;
                    linenoiseHide(&ls);
                    printf("count %d\n", counter++);
                    linenoiseShow(&ls);
#endif
                }
            }
            linenoiseEditStop(&ls);
            if (line == NULL) break;
            len = strlen(line);
            if (len > 0) {
                strcpy(input, line);
                linenoiseHistoryAdd(line);
            }
            linenoiseFree(line);
        } else {
            INFO(PROMPT);
            fflush(stdout);
            if (fgets(input, sizeof(input)/2, stdin) == NULL) break;
            len = strlen(input);
        }
        
        mark = amy_sysclock();
        //metro_info();
        
        if (len == 0) continue;
        
        char *token = strtok(input, splitter);

        while (token != NULL) {
            int n = token - input - 1;
            code = process(mark, token);
            token = strtok(NULL, splitter);
        }
    }

    //metro_stop();
    //thrd_join(t, NULL);

    amy_live_stop();
    
    if (use_linenoise) {
        linenoiseHistorySave(".rattle_history");
    }
    
    puts("rattle done");
    
    return 0;
}
