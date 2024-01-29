#ifndef _RATLIB_H_
#define _RATLIB_H_

void rat_device(int d);
void rat_list(void);
char *rat_scan(char *input);
void rat_start(void);
void rat_send(char *trimmed);
void rat_stop(void);
int rat_frames(void);
void rat_frame_stop(void);
void rat_frame_start(short int *sbuf, int slen, short int *n);
unsigned int rat_clock(void);

#endif
