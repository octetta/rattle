#include <ctype.h>
#include <string.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <signal.h>

#include "amy.h"
#include "rma.h"
#include "rattlefy.h"

int times[RATIO_TOP][RATIO_BOTTOM] = {{-1}};

int location[PAT_COUNT];
int playing[PAT_COUNT];
int modulus[PAT_COUNT] = {[0 ... 9] = 4};
char pattern[PAT_COUNT][SEQ_LEN][STORAGE_SIZE];

int get_modulus(int n) {
    return modulus[n];
}

char usrvar[IDENT_COUNT][STORAGE_SIZE] = {[0 ... IDENT_COUNT-1] = {[0 ... STORAGE_SIZE-1] = 0}};
int usrvar_int[IDENT_COUNT] = {[0 ... IDENT_COUNT-1] = 0};
double usrvar_fp[IDENT_COUNT] = {[0 ... IDENT_COUNT-1] = 0.0};
unsigned char usrvar_data[IDENT_COUNT][STORAGE_SIZE] = {[0 ... IDENT_COUNT-1] = {[0 ... STORAGE_SIZE-1] = 0}};

char sysvar[IDENT_COUNT][STORAGE_SIZE] = {[0 ... IDENT_COUNT-1] = {[0 ... STORAGE_SIZE-1] = 0}};
int sysvar_int[IDENT_COUNT] = {[0 ... IDENT_COUNT-1] = 0};
double sysvar_fp[IDENT_COUNT] = {[0 ... IDENT_COUNT-1] = 0.0};
unsigned char sysvar_data[IDENT_COUNT][STORAGE_SIZE] = {[0 ... IDENT_COUNT-1] = {[0 ... STORAGE_SIZE-1] = 0}};

void init_ratio(int quarter) {
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
        sprintf(sysvar[METRO_INDEX], "%d", QUARTER);
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

void basic_ms(int ms) {
    struct timespec dur;
    struct timespec rem;
    int sec = ms / 1000;
    int remaining_ms = ms - (sec * 1000);
    dur.tv_sec = sec;
    dur.tv_nsec = remaining_ms * 1000000;
    int n = nanosleep(&dur, &rem);
}

void (*loader_ms)(int ms) = basic_ms;

void set_loader_ms(void *fn) {
    loader_ms = fn;
}

int delayer(char *token, int start) {
    char ident = token[start+1];
    if (ident == '\0') {
        loader_ms(sysvar_int[METRO_INDEX]);
    }
    if (isnumber(ident)) {
        char *val = token+start+1;
        int next;
        int n = intgrabber(val, &next);
        loader_ms(n);
    }
    return 1;
}

int setmod(char *token, int start) {
    int n;
    char ident = token[start+1];
    if (ident == '\0') {
        for (int pat=0; pat<PAT_COUNT; pat++) {
            printf("%%%d=%d\n", pat, modulus[pat]);
        }
    }
    if (isnumber(ident)) {
        char *val = token+start+2;
        int pat = ident - '0';
        switch (*val) {
            case '\0':
                printf("%d\n", modulus[pat]);
                break;
            case '=':
                val++;
                if (isnumber(*val)) {
                    int next;
                    int n = intgrabber(val, &next);
                    if (n > 0) {
                        modulus[pat] = n;
                    }
                }
                break;
        }
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
                //INFO("%c%6d%c ", first, bufs[index][i], last);
                printf("%d ", bufs[index][i]);
                w++;
                if (w > 7) {
                    w = 0;
                    //INFO("\n");
                }
            }
            //if (w) INFO("\n");
            printf("\n");
            break;
        case METRO_SYM:
            ratio(1,1);
            for (int i=0; i<RATIO_TOP; i++) {
                for (int j=0; j<RATIO_BOTTOM; j++) {
                    INFO("_%d%d=%-4d ", i+1, j+1, ratio(i+1, j+1));
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

int raw_getter_int(int us, char ident) {
    int var = ident - IDENT_FIRST;
    if (us == USR) return usrvar_int[var];
    if (us == SYS) return sysvar_int[var];
    return -1;
}

int raw_setter(int us, char ident, char *val) {
    int var = ident - IDENT_FIRST;
    if (us == USR) {
        strcpy(usrvar[var], val);
        usrvar_int[var] = strtol(val, NULL, 10);
        usrvar_fp[var] = strtod(val, NULL);
    } else if (us == SYS) {
        strcpy(sysvar[var], val);
        sysvar_int[var] = strtol(val, NULL, 10);
        sysvar_fp[var] = strtod(val, NULL);
    }
    return 0;
}

void showpattern(int pat) {
    int count = 0;
    for (int i=0; i<SEQ_LEN; i++) {
        char *s = pattern[pat][i];
        if (*s == '\0') break;
        count++;
    }
    if (count == 0) return;
    printf("# :%d -> playing:%d location:%d(%d) %%:%d\n",
        pat, playing[pat], location[pat], count-1, modulus[pat]);
    for (int i=0; i<count; i++) {
        printf(":%d/%d=%s\n", pat, i, pattern[pat][i]);
    }
}

int actionpattern(char *token, int start) {
    char ident = token[start+1];
    char action = token[start+2];
    int pat = ident - '0';
    switch (action) {
        case '\0':
            showpattern(pat);
            break;
        case '+':
            //printf("play %d\n", pat);
            setplay(pat, 1);
            location[pat] = 0;
            break;
        case '-':
            //printf("stop %d\n", pat);
            setplay(pat, 0);
            break;
        case '.':
            //printf("resume %d\n", pat);
            setplay(pat, 1);
            break;
        case '/':
            if (isnumber(token[start+3])) {
                int next;
                int step = intgrabber(token + start + 3, &next);
                if (step < SEQ_LEN) {
                    char *s = pattern[pat][step];
                    char then = token[start + 3 + next];
                    switch (then) {
                        case '\0':
                            break;
                        case '=':
                            setpattern(pat, step, token+3+next+1);
                            break;
                    }
                }
            }
            break;
    }
    return 1;
}

int setgetsys(char *token, int start) {
    char ident = token[start+1];
    if (ident == '\0') {
        for (int n=0; n<IDENT_COUNT; n++) {
            if (strlen(sysvar[n]) > 0) {
                printf("$%c=%s\n", n+'a', sysvar[n]);
            }
        }
        return 1;
    }
    if (ident == ':') {
        for (int pat=0; pat<PAT_COUNT; pat++) {
            showpattern(pat);
        }
    }
    if (isnumber(ident)) return actionpattern(token, start);
    if (!isident(ident)) return 1;
    int index = ident - IDENT_FIRST;
    char action = token[start+2];
    if (action == '=') {
        char *val = token+3;
        raw_setter(SYS, ident, val);
        int n;
        switch (ident) {
            case METRO_SYM:
                n = intgrabber(val, NULL);
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
        for (int n=0; n<IDENT_COUNT; n++) {
            if (strlen(usrvar[n]) > 0) {
                printf("$%c=%s\n", n+'a', usrvar[n]);
            }
        }
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

int unit(unsigned int now, char *token) {
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
        case '%':
            return setmod(token, start);
        case QUERY: // query
            //VERBOSE("QUERY\n");
            return query(token, start);
        case VARIABLE: // user
            //VERBOSE("VARIABLE\n");
            return setgetusr(token, start);
        case SETTING: // system
            //VERBOSE("SETTING\n");
            return setgetsys(token, start);
        case 't':
            //VERBOSE("ABSTIME\n");
            tflag = 1;
            break;
        case '+':
            //VERBOSE("RELTIME\n");
            rflag = 1;
            break;
        case '_':
            //VERBOSE("RATIOTIME\n");
            mflag = 1;
            break;
        case CAPTURE:
            //VERBOSE("OTHER\n");
            return other(token, start);
        case COMMENT:
            //VERBOSE("COMMENT\n");
            return 1;
        case DELAY:
            return delayer(token, start);
            return 1;
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
    //VERBOSE("-> amy {%s}\n", final);
    
    amy_play_message(final);
    
    return 1;
}

char splitter[] = { SEPARATOR, '\n', '\0' };

int process(unsigned int mark, char *input) {
    char *copy = strdup(input);
    char *token = strtok(copy, splitter);
    int code = 1;
    while (token != NULL) {
        code = unit(mark, token);
        token = strtok(NULL, splitter);
    }
    free(copy);
    return code;
}

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

void setpattern(int pat, int step, char *s) {
    if (pat >= 0 && pat < PAT_COUNT && step >= 0 && step < SEQ_LEN) {
        strncpy(pattern[pat][step], s, STORAGE_SIZE);
        for (int i=0; i<STORAGE_SIZE; i++) {
            char c = pattern[pat][step][i];
            if (c == '\0') break;
            if (c == '&') pattern[pat][step][i] = ';';
        }
    }
}
void setstep(int pat, int step) {
    if (pat >= 0 && pat < PAT_COUNT && step >= 0 && step < SEQ_LEN) {
        location[pat] = step;
    }
}
void setplay(int pat, int play) {
    if (pat >= 0 && pat < PAT_COUNT) {
        playing[pat] = play;
    }
}

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

void init_looper(void) {
    int i;
    int j;
    for (i=0; i<PAT_COUNT; i++) {
        setstep(i, 0);
        setplay(i, 0);
        for (j=0; j<SEQ_LEN; j++) {
            setpattern(i, j, "");
        }
    }
}

void looper(unsigned int now) {
    int pat;
    amytimeb = amy_sysclock();
    gettimeofday(&ostime1, NULL);
    for (pat=0; pat<PAT_COUNT; pat++) {
        if (!playing[pat]) continue;
        if ((clockcounter % get_modulus(pat)) == 0) {
            int step = location[pat];
            if (strlen(pattern[pat][step]) > 0) {
                if (pattern[pat][step][0] == '/') {
                    // unconditional jump to 0 for now
                    // add other possibilities here later
                    step = 0;
                }
                // after possible step location changes process things
                if (strlen(pattern[pat][step]) > 0) {
                    process(now, pattern[pat][step]);
                }
                step++;
                location[pat] = step;
            } else {
                playing[pat] = 0;
            }
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


void loader(char *use_file) {
    FILE *in = NULL;
    in = fopen(use_file, "r");
    if (in) {
        char input[1024];
        int len = 0;
        //printf("reading from <%s>\n", use_file);
        while (1) {
            unsigned int mark = amy_sysclock();
            if (fgets(input, sizeof(input)/2, in) == NULL) break;
            len = strlen(input);
            if (len == 0) break;
            //printf("read <%.*s>\n", len-1, input);
            if (process(mark, input) == 0) break;
        }
        fclose(in);
    } else {
        printf("can't read from <%s>\n", use_file);
    }
}
