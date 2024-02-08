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
int rat_nchans(void);
int rat_frame_at(int n);
void rat_framer(int len);
unsigned int rat_clock(void);
int rat_oscs(void);
int rat_sample_rate(void);
int rat_block_size(void);
void rat_example(int n);

#endif
