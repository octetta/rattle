#ifndef _RATTLEFY_H_
#define _RATTLEFY_H_

#define RATIO_TOP (8)
#define RATIO_BOTTOM (8)
#define IDENT_FIRST 'a'
#define IDENT_LAST  'z'
#define IDENT_COUNT IDENT_LAST - IDENT_FIRST + 1

#define STORAGE_SIZE 64

#define PAT_COUNT 10
#define SEQ_LEN 100

extern int location[PAT_COUNT];
extern int playing[PAT_COUNT];
extern char pattern[PAT_COUNT][SEQ_LEN][STORAGE_SIZE];

#define QUARTER 250

#define METRO_SYM ('m')
#define METRO_INDEX (METRO_SYM - IDENT_FIRST)

#define INFO printf
#define VERBOSE printf

#define CLEN (44100)
#define BLEN (4)
#define SEPARATOR ';'

#define QUERY     '?'
#define VARIABLE  '$'
#define SETTING   ':'
#define CAPTURE   '<'
#define PATCH     '>'
#define PATTERN   '/'
#define COMMENT   '#'
#define DELAY     '~'

#define PROMPT    "# "

#define USR (0)
#define SYS (1)

int unit(unsigned int now, char *token);
int process(unsigned int now, char *input);

extern char splitter[];

//

int udp_open(int port);

void setpattern(int pat, int step, char *s);
void setstep(int pat, int step);
void setplay(int pat, int play);

int raw_setter(int us, char ident, char *val);
int raw_getter_int(int us, char ident);

int get_modulus(int n);

void init_looper(void);
void looper(unsigned int now);
void motor_init(int ms);

extern unsigned int interval_amy;
extern struct timeval interval_os;
extern unsigned int loop_amy;
extern struct timeval loop_os;

void loader(char *use_file);

void set_loader_ms(void *fn);

#endif
