#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#if 0
#include <sys/select.h>
#else
#include <poll.h>
#endif
#include <signal.h>

#include "amy.h"
#include "rma.h"
#include "rattlefy.h"
#include "linenoise.h"

static int use_linenoise = 1;

// measure the latency between metro match messages from AMY
unsigned int amytimea = 0;
unsigned int amytimeb = 0;
unsigned int interval_amy = 0;
unsigned int loop_amy = 0;
// same for os time
struct timeval ostime0;
struct timeval ostime1;
struct timeval ostime2;
struct timeval loop_os;
struct timeval interval_os;

static int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y) {
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    return x->tv_sec < y->tv_sec;
}

// eventually put old logic for loops here, just mashed a message now

int looppos = 0;
int clockcounter = 0;

void looper(unsigned int now) {
    static char first = 1;
    int i;
    int j;
    if (first) {
        first = 0;
        for (i=0; i<PAT_COUNT; i++) {
            setstep(i, 0);
            setplay(i, 0);
            for (j=0; j<SEQ_LEN; j++) {
                setpattern(i, j, "");
            }
        }
        //setpattern(0, 0, "v0l1&_12v0l0");
        //setpattern(0, 1, "v0l1&_12v0l0");
        setpattern(0, 0, "v0l1");
        setpattern(0, 1, "v0l0");
        setpattern(0, 2, "/");
        //setpattern(1, 0, "v10l1");
        //setpattern(1, 1, "#");
        //setpattern(1, 2, "v10l0");
        //setpattern(1, 3, "#");
        //setpattern(1, 4, "/");
        
        setstep(0, 0);
        setstep(1, 0);

        setplay(0, 1);
        setplay(1, 1);
    }
    amytimeb = amy_sysclock();
    gettimeofday(&ostime1, NULL);
    for (i=0; i<PAT_COUNT; i++) {
        if (!playing[i]) continue;
        int n = location[i];
        if (strlen(pattern[i][n]) > 0) {
            if (pattern[i][n][0] == '/') n = 0;
            if (strlen(pattern[i][n]) > 0) {
#if 0
                amy_play_message(pattern[i][n]);
#else
                process(now, pattern[i][n]);
#endif
            }
            n++;
            location[i] = n;
        } else {
            playing[i] = 0;
        }
    }
    loop_amy = amy_sysclock() - amytimeb;
    gettimeofday(&ostime2, NULL);
    interval_amy = amytimeb - amytimea;
    amytimea = amytimeb;
    timeval_subtract(&loop_os, &ostime2, &ostime1); // time from top of pattern to bottom
    timeval_subtract(&interval_os, &ostime1, &ostime0); // time from last looper call
    memcpy(&ostime0, &ostime1, sizeof(struct timeval));
    clockcounter++;
}

#ifdef MACFAKETIMER
#include "macos-timer.h"
#endif

static struct sigevent motor_event;
static timer_t motor_timer;
static struct itimerspec motor_period;

void motor_init(int ms);

void motor_wheel(union sigval timer_data) {
#if 1
    static int interval = -1;
    if (interval <= 0) {
        interval = raw_getter_int(SYS, 'm');
    }
    static unsigned int last = 0;
    unsigned int now = amy_sysclock();
    if (now > last) {
        //write(1, ".", 1);
        looper(now);
        interval = raw_getter_int(SYS, 'm');
        last = now + interval;
    }
#else
    static int interval = -1;
    if (interval <= 0) {
        interval = raw_getter_int(SYS, 'm');
    }
    looper(1);
    int new_interval = raw_getter_int(SYS, 'm');
    if (interval != new_interval) {
        interval = new_interval;
        timer_delete(motor_timer);
        motor_init(new_interval);
    }
#endif
}

static unsigned int motor_data = 42;

void motor_init(int ms) {
    int sec = ms / 1000;
    int nsec = (ms - (sec * 1000)) * 1000000;
    motor_event.sigev_notify = SIGEV_THREAD;
    motor_event.sigev_notify_function = motor_wheel;
    motor_event.sigev_value.sival_ptr = (void *)&motor_data;
    motor_event.sigev_notify_attributes = NULL;
    timer_create(CLOCK_REALTIME, &motor_event, &motor_timer);
    //timer_create(CLOCK_MONOTONIC, &motor_event, &motor_timer);
    motor_period.it_value.tv_sec = sec;
    motor_period.it_value.tv_nsec = nsec;
    motor_period.it_interval.tv_sec = sec;
    motor_period.it_interval.tv_nsec = nsec;
    timer_settime(motor_timer, 0, &motor_period, NULL);
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
        int maxfd = ls.ifd;
        r = select(maxfd+1, &readfds, NULL, NULL, &tv);
#else
        struct pollfd p[1];
        p[0].fd = ls.ifd;
        p[0].events = POLLIN;
        //p[1].fd = serve_fd;
        r = poll(p, 1, 1000);
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
    
    raw_setter(SYS, 'm', "250");
#if 1
    motor_init(2);
#else
    motor_init(250);
#endif

    amy_start(/* cores= */ 1, /* reverb= */ 1, /* chorus= */ 1);
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
