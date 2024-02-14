#include <stdio.h>
#include <string.h>

#include "amy.h"
//#include "pcm_tiny.h"
#include "examples.h"
#include "rma.h"

#define SLEN (44100)

void delay_ms(uint32_t ms) {
    uint32_t start = amy_sysclock();
    while(amy_sysclock() - start < ms) usleep(THREAD_USLEEP);
}

void rat_debug(int d) {
    show_debug(d);
}

void rat_global_dump(void) {
    printf("global: volume %f eq: %f %f %f \n", amy_global.volume, S2F(amy_global.eq[0]), S2F(amy_global.eq[1]), S2F(amy_global.eq[2]));
}

void rat_osc_dump_one(int i) {
    //printf("osc %d: status %d wave %d mod_target %d mod_source %d velocity %flogratio %f feedback %f resonance %f step %f chained %d algo %d source %d,%d,%d,%d,%d,%d  \n",
    //        i, synth[i].status, synth[i].wave, synth[i].mod_target, synth[i].mod_source,
    //        synth[i].velocity, synth[i].logratio, synth[i].feedback, synth[i].resonance, P2F(synth[i].step), synth[i].chained_osc, 
    //        synth[i].algorithm, 
    //        synth[i].algo_source[0], synth[i].algo_source[1], synth[i].algo_source[2], synth[i].algo_source[3], synth[i].algo_source[4], synth[i].algo_source[5] );
    printf("v%d", i);
    printf("w%d", synth[i].wave);
    if (synth[i].mod_target) printf("g%d", synth[i].mod_target);
    if (synth[i].mod_source != 65535) printf("L%d", synth[i].mod_source);
    printf("l%g", synth[i].velocity);
    if (synth[i].feedback) printf("b%g", synth[i].feedback);
    printf("R%g", synth[i].resonance);
    if (synth[i].chained_osc != 65535) printf("c%d", synth[i].chained_osc);
    if (synth[i].algorithm != 0) printf("o%d", synth[i].algorithm);
    if (synth[i].algo_source[0] != 32767) printf("O%d", synth[i].algo_source[0]);
    if (synth[i].algo_source[1] != 32767) printf(",%d", synth[i].algo_source[1]);
    if (synth[i].algo_source[2] != 32767) printf(",%d", synth[i].algo_source[2]);
    if (synth[i].algo_source[3] != 32767) printf(",%d", synth[i].algo_source[3]);
    if (synth[i].algo_source[4] != 32767) printf(",%d", synth[i].algo_source[4]);
    if (synth[i].algo_source[5] != 32767) printf(",%d", synth[i].algo_source[5]);
    for(uint8_t j=0;j<MAX_BREAKPOINT_SETS;j++) {
        char a = 'T';
        char b = 'A';
        if (j == 1) {
            a = 'W';
            b = 'B';
        }
        if (synth[i].breakpoint_target[j]) {
            printf("%c%d", a, synth[i].breakpoint_target[j]);
            for(uint8_t k=0;k<MAX_BREAKPOINTS;k++) {
                if (k == 0) printf("%c", b);
                if (synth[i].breakpoint_times[j][k] == -1) {
                    break;
                } else {
                    if (k > 0) printf(",");
                    printf("%d,%g", synth[i].breakpoint_times[j][k], synth[i].breakpoint_values[j][k]);
                }
            }
        }
    }
    puts("");

    printf("mod osc %d a%g f%g d%g F%g R%g b%g Q%g \n",
        i,
        msynth[i].amp,
        msynth[i].logfreq,
        msynth[i].duty,
        msynth[i].filter_logfreq,
        msynth[i].resonance,
        msynth[i].feedback,
        msynth[i].pan);
    
    printf("a%g,%g,%g,%g,%g,%g\n", synth[i].amp_coefs[0], synth[i].amp_coefs[1], synth[i].amp_coefs[2], synth[i].amp_coefs[3], synth[i].amp_coefs[4], synth[i].amp_coefs[5]);
    printf("f%g,%g,%g,%g,%g,%g\n", synth[i].logfreq_coefs[0], synth[i].logfreq_coefs[1], synth[i].logfreq_coefs[2], synth[i].logfreq_coefs[3], synth[i].logfreq_coefs[4], synth[i].logfreq_coefs[5]);
    printf("F%g,%g,%g,%g,%g,%g\n", synth[i].filter_logfreq_coefs[0], synth[i].filter_logfreq_coefs[1], synth[i].filter_logfreq_coefs[2], synth[i].filter_logfreq_coefs[3], synth[i].filter_logfreq_coefs[4], synth[i].filter_logfreq_coefs[5]);
    printf("d%g,%g,%g,%g,%g,%g\n", synth[i].duty_coefs[0], synth[i].duty_coefs[1], synth[i].duty_coefs[2], synth[i].duty_coefs[3], synth[i].duty_coefs[4], synth[i].duty_coefs[5]);
    printf("Q%g,%g,%g,%g,%g,%g\n", synth[i].pan_coefs[0], synth[i].pan_coefs[1], synth[i].pan_coefs[2], synth[i].pan_coefs[3], synth[i].pan_coefs[4], synth[i].pan_coefs[5]);
    if (0) {
        for(uint8_t j=0;j<MAX_BREAKPOINT_SETS;j++) {
            printf("  bp%d (target %d): ", j, synth[i].breakpoint_target[j]);
            for(uint8_t k=0;k<MAX_BREAKPOINTS;k++) {
                printf("%" PRIi32 ": %f ", synth[i].breakpoint_times[j][k], synth[i].breakpoint_values[j][k]);
            }
            printf("\n");
        }
        printf("mod osc %d: amp: %f, logfreq %f duty %f filter_logfreq %f resonance %f fb/bw %f pan %f \n", i, msynth[i].amp, msynth[i].logfreq, msynth[i].duty, msynth[i].filter_logfreq, msynth[i].resonance, msynth[i].feedback, msynth[i].pan);
    }
}

void rat_osc_dump(int i) {
    rat_osc_dump_one(i);
    int n = synth[i].chained_osc;
    if (n != 65535) {
        rat_osc_dump(n);
    }
    int l[6];
    for (int j=0; j<6; j++) {
        l[j] = -1;
    }
    for (int j=0; j<6; j++) {
        int o = synth[i].algo_source[j];
        if (o != 32767) rat_osc_dump(o);
        else break;
    }
}

#define MAX_VOICES 16 // ripped from patches.c
extern uint8_t osc_to_voice[AMY_OSCS];
extern uint16_t voice_to_base_osc[MAX_VOICES];

void rat_v2r(void) {
    printf("v2r => ");
    for (int i=0; i<AMY_OSCS; i++) {
        int x = osc_to_voice[i];
        if (x != 255) printf("v%d:r%d ", i, x);
    }
    puts("");
}

void rat_r2v(void) {
    printf("r2v => ");
    for (int i=0; i<MAX_VOICES; i++) {
        int x = voice_to_base_osc[i];
        if (x != 65535) printf("r%d:v%d ", i, x);
    }
    puts("");
}

void rat_osc_multi(int f) {
    if (f == 0) {
        printf("off => ");
        for (int i=0; i<AMY_OSCS; i++) {
            if (synth[i].status == OFF) {
                //rat_osc_dump(i);
                printf("%d ", i);
            }
        }
        puts("");
    } else if (f == 1) {
        printf(" on => ");
        for (int i=0; i<AMY_OSCS; i++) {
            if (synth[i].status != OFF) {
                //rat_osc_dump(i);
                printf("%d ", i);
            }
        }
        puts("");
    }
}

void rat_device(int d) {
    amy_device_id = d;
}

void rat_list(void) {
    amy_print_devices();
}

void rat_example(int n) {
#if 1
    switch (n) {
        case 0:
            example_reverb();
            break;
        case 1:
            example_chorus();
            break;
        case 2:
            example_ks(amy_sysclock());
            break;
        case 3:
            bleep(amy_sysclock());
            break;
        case 4:
            //example_fm(amy_sysclock());
            break;
        case 5:
            example_multimbral_fm(amy_sysclock(), 10);
            break;
        case 6:
            example_drums(amy_sysclock(), 8);
            break;
        case 7:
            example_sine(amy_sysclock());
            break;
        case 8:
            example_patches();
            break;
        case 9:
            //example_juno_chord();
            break;
        case 10:
            //example_dx7_chord();
            break;
        case 11:
            example_voice_alloc();
            break;
    }
#endif
}

char *rat_scan(char *input) {
    int len = strlen(input);
    if (len == 0) return input;
    char *first = input;
    int scanning = 1;
    while (scanning) {
        switch (*first) {
            case '#':
            case ';':
                *first = '\0';
            case '\0':
                scanning = 0;
                break;
        }
        first++;
    }
    char *trimmed = input;
    int trimming = 1;
    while (trimming) {
        switch (*trimmed) {
            case ' ':
            case '\n':
            case '\t':
                trimmed++;
                break;
            default:
                trimming = 0;
                break;
        }
    }
    len = strlen(trimmed);
    trimming = 1;
    char *rtrim = trimmed + len - 1;
    while (trimming) {
        if (rtrim == trimmed) break; 
        switch (*rtrim) {
            case ' ':
            case '\n':
            case '\t':
                *rtrim = '\0';
                rtrim--;
                break;
            default:
                trimming = 0;
                break;
        }
    }
    return trimmed;
}

extern unsigned char computed_delta_set;

void rat_start(void) {
    //amy_restart();
    //computed_delta_set = 0;
    amy_start(/* cores= */ 1, /* reverb= */ 1, /* chorus= */ 1);
    amy_live_start();
    amy_reset_oscs();
}

void rat_send(char *trimmed) {
    amy_play_message(trimmed);
}

static short int *frames = NULL;
static unsigned int nframes = 0;

void rat_stop(void) {
    capture_stop();
    amy_live_stop();
    if (frames) free(frames);
    frames = NULL;
}

int rat_frames(void) {
    return captured_frames();
}

void rat_frame_stop(void) {
    capture_stop();
}

void rat_frame_start(short int *sbuf, int slen, short int *n) {
    capture_start(sbuf, slen, n);
}

unsigned int rat_clock(void) {
    return amy_sysclock();
}

int rat_nchans(void) {
    return AMY_NCHANS;
}

int rat_oscs(void) {
    return AMY_OSCS;
}

int rat_sample_rate(void) {
    return AMY_SAMPLE_RATE;
}

int rat_block_size(void) {
    return AMY_BLOCK_SIZE;
}

short int rat_frame_at(int n) {
    if (frames && n > 0 && n < captured_frames()) {
        return frames[n];
    }
    return 0;
}

void rat_framer(int len) {
    //printf("rat_framer = %d\n", len);
    capture_stop();
    if (frames) free(frames);
    frames = (short int *)malloc(len * sizeof(short int));
    //printf("frames = %p\n", frames);
    nframes = len;
    capture_start(frames, len, NULL);
}
