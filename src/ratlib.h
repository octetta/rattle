#ifndef _RATLIB_H_
#define _RATLIB_H_

void rat_patch_from_framer(int p);
void rat_patch_show(int n);
void rat_v2r(void);
void rat_r2v(void);
void rat_osc_multi(int f);
void rat_global_dump(void);
void rat_osc_dump(int i);
void rat_debug(int d);
void rat_see(void);
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
