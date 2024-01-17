#ifndef _RATTLEFY_H_
#define _RATTLEFY_H_

#define RATIO_TOP (8)
#define RATIO_BOTTOM (8)
#define IDENT_FIRST 'a'
#define IDENT_LAST  'z'
#define IDENT_COUNT IDENT_LAST - IDENT_FIRST + 1

#define STORAGE_SIZE 64
#define QUARTER 250

#define METER_SYM ('m')
#define METER_INDEX (METER_SYM - IDENT_FIRST)

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

#define PROMPT    "# "

#define USR (0)
#define SYS (1)

int process(unsigned int now, char *token, char sep);

extern char splitter[];
#endif
