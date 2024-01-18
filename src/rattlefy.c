#include <ctype.h>
#include <string.h>
#include <sys/time.h>

#include "amy.h"
#include "rma.h"
#include "rattlefy.h"
#include "tinycthread.h"

int times[RATIO_TOP][RATIO_BOTTOM] = {{-1}};

char usrvar[IDENT_COUNT][STORAGE_SIZE] = {[0 ... IDENT_COUNT-1] = {[0 ... STORAGE_SIZE-1] = 0}};
char sysvar[IDENT_COUNT][STORAGE_SIZE] = {[0 ... IDENT_COUNT-1] = {[0 ... STORAGE_SIZE-1] = 0}};

int usrvar_int[IDENT_COUNT] = {[0 ... IDENT_COUNT-1] = 0};
double usrvar_fp[IDENT_COUNT] = {[0 ... IDENT_COUNT-1] = 0.0};
unsigned char usrvar_data[IDENT_COUNT][STORAGE_SIZE] = {[0 ... IDENT_COUNT-1] = {[0 ... STORAGE_SIZE-1] = 0}};

int metro_quarter = QUARTER;

void init_ratio(int quarter) {
    metro_quarter = quarter;
    for (int i=0; i<RATIO_TOP; i++) {
        for (int j=0; j<RATIO_BOTTOM; j++) {
            times[i][j] = (quarter * 4) * (i+1) / (j+1);
        }
    }
}

int ratio(int a, int b) {
    if (a > RATIO_TOP) return QUARTER;
    if (b > RATIO_BOTTOM) return QUARTER;

    if (a < 1) return QUARTER;
    if (b < 1) return QUARTER;

    if (times[0][0] == -1) {
        init_ratio(QUARTER);
        sprintf(sysvar[RATIO_INDEX], "%d", QUARTER);
    }
    return times[a-1][b-1];
}

int intgrabber(char *token, unsigned int *next) {
    int n = 0;
    char *keep = token;
    while (1) {
        char c = *token;
        if (c >= '0' && c <= '9') {
            int i = c - '0';
            n = n*10 + i;
        } else break;
        token++;
    }
    if (next) *next = token - keep;
    return n;
}

unsigned int clen = CLEN;
unsigned int bptr = 0;
short *bufs[BLEN] = {NULL, NULL, NULL, NULL};
unsigned int lens[BLEN] = {0, 0, 0, 0};
short chans[BLEN] = {AMY_NCHANS, AMY_NCHANS, AMY_NCHANS, AMY_NCHANS};

char isident(char ident) {
    if (ident < IDENT_FIRST) return 0;
    if (ident > IDENT_LAST) return 0;
    return 1;
}

char isnumber(char c) {
    switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return 1;
        default:
            return 0;
    }
}

int other(char *token, int start) {
    int frames = 0;
    switch (token[start]) {
        case CAPTURE: // start capture
            capture_stop();
            lens[bptr] = captured_frames();
            bptr++;
            if (bptr >= BLEN) bptr = 0;
            if (bufs[bptr]) free(bufs[bptr]);
            if (isnumber(token[1])) {
                frames = intgrabber(token+1, NULL);
            } else {
                frames = CLEN;
            }
            if (frames > 0) {
                bufs[bptr] = malloc(frames * sizeof(short));
                capture_start(bufs[bptr], frames, NULL);
            }
            break;
    }
    return 1;
}

int query(char *token, int start) {
    switch (token[start+1]) {
        case 'c':
            INFO("%" PRIu32 "\n", amy_sysclock());
            break;
        case 's':
            INFO("%" PRIu32 "\n", total_samples);
            break;
        case 'r': // show capture count
            for (unsigned int i=0; i<BLEN; i++) {
                char c = ' ';
                unsigned int len = lens[i];
                if (i == bptr) {
                    c = '*';
                    len = captured_frames();
                }
                INFO("%c [%d](%d)%d\n", c, i, chans[i], len);
            }
            break;
        case 'd': // show capture data
            int w = 0;
            unsigned int frames = 0;
            unsigned int index = bptr;
            short nchan = AMY_NCHANS;
            char c = token[start+2];
            if (c == '\0') {
                frames = captured_frames();
            } else {
                if (isnumber(c)) {
                    index = c - '0';
                    if (index < BLEN) {
                        frames = lens[index];
                        nchan = chans[index];
                    }
                } else frames = 0;
            }
            INFO("?d%c / [%d] (%d) = %d\n", c, index, nchan, frames);
            char first = ' ';
            char last = ' ';
            int which = 0;
            for (unsigned int i=0; i<frames; i++) {
                if (which == 0) {
                    first = '[';
                    last = ',';
                } else if (which == nchan-1) {
                    first = ' ';
                    last = ']';
                } else {
                    first = ' ';
                    last = ',';
                }
                which++;
                if (which >= nchan) which = 0;
                INFO("%c%6d%c ", first, bufs[index][i], last);
                w++;
                if (w > 7) {
                    w = 0;
                    INFO("\n");
                }
            }
            if (w) INFO("\n");
            break;
        case RATIO_SYM:
            ratio(1,1);
            for (int i=0; i<RATIO_TOP; i++) {
                for (int j=0; j<RATIO_BOTTOM; j++) {
                    INFO("_%d%d => %-5d ; ", i+1, j+1, ratio(i+1, j+1));
                }
                INFO("\n");
            }
            break;
        default:
            INFO("?\n");
            break;
    }
    return 1;
}

static int _setter(int us, char ident, char *val) {
    int var = ident - IDENT_FIRST;
    if (us == USR) {
        strcpy(usrvar[var], val);
    } else if (us == SYS) {
        strcpy(sysvar[var], val);
    }
    return 0;
}

int setgetsys(char *token, int start) {
    char ident = token[start+1];
    if (ident == '\0') {
        INFO("show all\n");
        return 1;
    }
    if (!isident(ident)) return 1;
    int index = ident - IDENT_FIRST;
    char action = token[start+2];
    if (action == '=') {
        char *val = token+3;
        _setter(SYS, ident, val);
        switch (ident) {
            case RATIO_SYM:
                int n = intgrabber(val, NULL);
                if (n > 0) {
                    init_ratio(n);
                    sprintf(sysvar[index], "%d", n);
                }
                break;
            default:
                break;
        }
    } else if (action == '\0' || action == ' ') {
        switch (ident) {
            case 'q':
                // quits
                return 0;
            default:
                INFO("%s\n", sysvar[index]);
                break;
        }
        return 1;
    }
    return 1;
}

int setgetusr(char *token, int start) {
    char ident = token[start+1];
    if (ident == '\0') {
        INFO("show all\n");
        return 1;
    }
    if (!isident(ident)) return 1;
    int index = ident - IDENT_FIRST;
    char action = token[start+2];
    if (action == '\0' || action == ' ') {
        INFO("%s\n", usrvar[index]);
        return 1;
    }
    if (action == '=') {
        strcpy(usrvar[index], token + 3);
    }
    return 1;
}

int process(unsigned int now, char *token) {
    char output[1024];
    int tflag = 0;
    int rflag = 0;
    int mflag = 0;
    unsigned int start = 0;
    int ntime = 0;
    // skip spaces at beginning
    while (1) {
        if (token[start] == ' ') start++;
        else break;
    }
    unsigned int len = strlen(token + start);
    // trim spaces at end
    unsigned int end = start + len - 1;
    while (1) {
        if (token[end] == ' ') { // here
            end--;
        } else {
            break;
        }
    }

    switch (token[start]) {
        case QUERY: // query
            return query(token, start);
            break;
        case VARIABLE: // user
            return setgetusr(token, start);
            break;
        case SETTING: // system
            return setgetsys(token, start);
            break;
        case 't':
            tflag = 1;
            break;
        case '+':
            rflag = 1;
            break;
        case '_':
            mflag = 1;
            break;
        case CAPTURE:
            return other(token, start);
            break;
        case COMMENT:
            return 1;
            break;
    }

    if (tflag || rflag) {
        start++;
        while (start <= end) {
            if (token[start] >= '0' && token[start] <= '9') {
                ntime = ntime*10 + token[start] - '0';
            } else break;
            start++;
        }
    } else if (mflag) {
        int a = token[start+1] - '0';
        int b = token[start+2] - '0';
        int c = ratio(a, b);
        if (c < 0) {
            return -1;
        }
        start += 3;
        ntime = c;
    }
    
    if (rflag || mflag) {
        ntime += now;
    }
    
    if (ntime == 0) {
        ntime = now;
    }
    
    sprintf(output, "t%d", ntime);
    // prior to user variables
    strncat(output, token+start, end-start+1); // here also
    // expand user variables
    char final[1024] = "";
    unsigned int olen = strlen(output);
    int vcnt = 0;
    for (unsigned int i=0; i<olen; i++) {
        if (output[i] == VARIABLE) {
            i++;
            char c = output[i];
            if (c >= IDENT_FIRST && c <= IDENT_LAST) {
                int var = c - IDENT_FIRST;
                strcat(final, usrvar[var]);
                vcnt++;
            }
        } else {
            int len = strlen(final);
            final[len] = output[i];
            final[len+1] = '\0';
        }
    }
    //VERBOSE("(%d/%d)=<%s>\n", now, vcnt, final);
    VERBOSE("-> amy {%s}\n", final);
    
    amy_play_message(final);
    
    return 1;
}

char splitter[] = { SEPARATOR, '\n', '\0' };

static int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y) {
    /* Perform the carry for the later subtraction by updating y. */
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

    /* Compute the time remaining to wait.
    tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

int metro_run = 1;
//                      999999999 nearly 1 sec
//                     1000000000 1 sec
//                      500000000 1/2 sec
//                      250000000 1/4
//                      125000000 1/8
//                       62500000 1/16
//                       31250000 1/32
//                       15625000 1/64
//                        7812500 1/128
//                        3906250 1/256
//                        1953125 1/512
//                        1000000 1/1000 (ms)
//                         500000 1/2000 (1/2 ms)
#define METRO             1000000
#define DELTA 10
unsigned int metro_nsec = METRO;
unsigned int metro_period = METRO / 1000;
struct timeval metro_tv;
int metro_res = 0;
int metro_a = 0;
int metro_b = 0;
//int metro_quarter = QUARTER;

int metro(void *arg) {
    struct timespec t0;
    struct timespec t1;
    struct timeval tv0;
    struct timeval tv1;
    metro_a = amy_sysclock() + metro_quarter;
    while (metro_run) {
        gettimeofday(&tv0, NULL);
        t0.tv_sec = 0;
        t0.tv_nsec = metro_nsec;
        while (metro_run) {
            thrd_sleep(&t0, &t1);
            if (t1.tv_nsec > 0) {
                break;
            }
            t0.tv_sec = 0;
            t0.tv_nsec = t1.tv_nsec;
        }
        gettimeofday(&tv1, NULL);
        metro_res = timeval_subtract(&metro_tv, &tv1, &tv0);
        if (metro_tv.tv_usec > metro_period) {
            metro_nsec -= (metro_tv.tv_usec / 4);
        } else if (metro_tv.tv_usec < metro_period) {
            metro_nsec += (metro_tv.tv_usec / 4);
        }
        if (amy_sysclock() > metro_a) {
            //write(2, ".", 1);
            metro_b = metro_a;
            metro_a = amy_sysclock() + metro_quarter;
        }
    }
    puts("metro done");
}

void metro_info(void) {
    printf("usec:%d sleep:%d diff:%d _14:%d\n",
        metro_tv.tv_usec,
        metro_nsec,
        metro_a-metro_b,
        metro_quarter);
}

void metro_stop(void) {
    metro_run = 0;
}
