// libminiaudio-audio.h

#ifndef __LIBMINIAUDIO_AUDIO_H
#define __LIBMINIAUDIO_AUDIO_H
#include "amy.h"
#include <pthread.h>

extern int16_t amy_channel;
extern int16_t amy_device_id;
extern uint8_t amy_running;

void amy_print_devices();
void amy_live_start();
void amy_live_stop();
void capture_start(short *buf, unsigned int frames, short *channels);
unsigned int captured_frames(void);
void capture_stop(void);
void set_signal_fd(int fd);
void set_frame_match(unsigned int n);
#endif