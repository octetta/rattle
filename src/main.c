#include <stdio.h>
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

// for pipes used to communicate between AMY and the poll multiplex here
int timing[2];

// measure the latency between frame match messages from AMY
unsigned int timea = 0;
unsigned int timeb = 0;
unsigned int timec = 0;


// eventually put old logic for loops here, just mashed a message now
int looppos = 0;
int clockcounter = 0;
void looper(void) {
    static char first = 1;
    int i;
    int j;
    if (first) {
        first = 0;
        for (i=0; i<PAT_COUNT; i++) {
            location[i] = 0;
            playing[i] = 0;
            for (j=0; j<SEQ_LEN; j++) {
                pattern[i][j][0] = '\0';
            }
        }
        playing[0] = 1;
        strcpy(pattern[0][0], "v0l1");
        strcpy(pattern[0][1], "v0l0");
        strcpy(pattern[0][2], "/");
        playing[1] = 1;
        strcpy(pattern[1][0], "v10l1");
        strcpy(pattern[1][1], "#");
        strcpy(pattern[1][2], "v10l0");
        strcpy(pattern[1][3], "#");
        strcpy(pattern[1][4], "/");
    }
    timeb = amy_sysclock();
    timec = timeb - timea;
    timea = timeb;
    for (i=0; i<PAT_COUNT; i++) {
        if (!playing[i]) continue;
        int n = location[i];
        if (strlen(pattern[i][n]) > 0) {
            if (pattern[i][n][0] == '/') n = 0;
            if (strlen(pattern[i][n]) > 0) {
#if 0
                amy_play_message(pattern[i][n]);
#else
                process(timeb, pattern[i][n]);
#endif
            }
            n++;
            location[i] = n;
        } else {
            playing[i] = 0;
        }
    }
    clockcounter++;
}

// multiplexer between linenoise and looper... needs a fallback for fgets?

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
        FD_SET(timing[0], &readfds);
        int maxfd = (ls.ifd > timing[0]) ? (ls.ifd) : (timing[0]);
        r = select(maxfd+1, &readfds, NULL, NULL, &tv);
#else
        struct pollfd p[2];
        p[0].fd = ls.ifd;
        p[0].events = POLLIN;
        //p[1].fd = serve_fd;
        p[1].fd = timing[0];
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
                char buf[1024];
                int n = read(timing[0], buf, sizeof(buf));
                if (n > 0) looper();
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
    
    // [0] = me
    // [1] = rma
    pipe(timing);

    set_signal_fd(timing[1]);
    set_frame_match(441*40);

    amy_start(/* cores= */ 1, /* reverb= */ 0, /* chorus= */ 0);
    amy_live_start();
    amy_reset_oscs();
    int code = 1;
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
        
        printf("timec:%d\n", timec);

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
