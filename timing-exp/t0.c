#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#if 0
//#define MA_NO_DECODING
//#define MA_NO_ENCODING
//#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
//#define MA_DEBUG_OUTPUT

#ifdef __APPLE__
    #define MA_NO_RUNTIME_LINKING
#endif
#define MINIAUDIO_IMPLEMENTATION

#if 1
// this shows the most output options under mxlinux
#define MA_NO_PULSEAUDIO
#define MA_NO_JACK
#else
// this shows the least output options under mxlinux
//#define MA_NO_PULSEAUDIO
//#define MA_NO_JACK
#endif

#endif

#include "miniaudio.h"

#define DEVICE_FORMAT       ma_format_s16
#define AMY_NCHANS 2
#define AMY_SAMPLE_RATE 44100
#define AMY_OK 0

ma_device_config deviceConfig;
ma_device device;
unsigned char _custom[4096];
ma_context context;
ma_device_info* pPlaybackInfos;
ma_uint32 playbackCount;
ma_device_info* pCaptureInfos;
ma_uint32 captureCount;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frame_count);

int print_devices() {
    ma_context context;
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        fprintf(stdout,"Failed to setup context for device list.\n");
        return -1;
    }

    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
        fprintf(stdout,"[[-1, \"Failed to get device list.\"]]\n");
        return -1;
    }

    char c = ',';
    fprintf(stdout, "[");
    for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
        if (iDevice == playbackCount -1) c = ']';
        fprintf(stdout,"[%d, \"%s\"]%c\n",
            iDevice, pPlaybackInfos[iDevice].name, c);
    }
    c = ',';
    fprintf(stdout, "[");
    for (ma_uint32 iDevice = 0; iDevice < captureCount; iDevice += 1) {
        if (iDevice == captureCount -1) c = ']';
        fprintf(stdout,"[%d, \"%s\"]%c\n",
            iDevice, pCaptureInfos[iDevice].name, c);
    }

    ma_context_uninit(&context);
    return 0;
}

int amy_device_id = -1;

int miniaudio_init() {
    if (amy_device_id < 0) {
        amy_device_id = 0;
    }

    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        printf("Failed to setup context for device list.\n");
        exit(1);
    }

    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
        printf("Failed to get device list.\n");
        exit(1);
    }
    
    if (amy_device_id >= (int32_t)playbackCount) {
        printf("invalid playback device\n");
        exit(1);
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    
    deviceConfig.playback.pDeviceID = &pPlaybackInfos[amy_device_id].id;
    deviceConfig.playback.format   = DEVICE_FORMAT;
    deviceConfig.playback.channels = AMY_NCHANS;
    deviceConfig.sampleRate        = AMY_SAMPLE_RATE;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = _custom;
    
    if (ma_device_init(&context, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        exit(1);
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        exit(1);
    }
    return AMY_OK;
}

unsigned int cbc = 0;
unsigned int fc = 0;

typedef struct {
    char valid;
    struct timespec t0;
    unsigned int cbc;
    unsigned int fc;
} tracker_t;

#define STC 64
#define STM (STC-1)

tracker_t st[STC];
char sp = 0;

//#define CT CLOCK_PROCESS_CPUTIME_ID
#define CT CLOCK_REALTIME

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frame_count) {
    clock_gettime(CT, &st[sp&STM].t0);
    st[sp&STM].valid = 1;
    st[sp&STM].cbc = cbc;
    st[sp&STM].fc = fc;
    sp++;
    cbc++;
    fc = frame_count;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        print_devices();
        exit(1);
    }
    amy_device_id = atoi(argv[1]);
    printf("using device %d\n", amy_device_id);
    for (int i=0; i<STC; i++) {
        st[i].valid = 0;
    }
    struct timespec starter;
    clock_gettime(CT, &starter);
    miniaudio_init();
    double tm = 0.0;
    int tf = 0;
    while (1) {
        printf("cbc:%d fc:%d (=%d)\n", cbc, fc, cbc*fc*2);
        tm = 0.0;
        tf = 0;
        for (int i=0; i<STC; i++) {
            char c = ' ';
            int now = i;
            int prev = i-1;
            if (prev < 0) {
                prev = STM;
            }
            if (st[now].cbc < st[prev].cbc) c = '*';
            unsigned int n = 0;
            double m = 0;
            if (st[now].valid && st[prev].valid) {
                if (c == ' ') {
                    struct timespec temp;
                    if ((st[now].t0.tv_nsec-st[prev].t0.tv_nsec)<0) {
                        temp.tv_sec = st[now].t0.tv_sec-st[prev].t0.tv_sec-1;
                        temp.tv_nsec = 1000000000+st[now].t0.tv_nsec-st[prev].t0.tv_nsec;
                    } else {
                        temp.tv_sec = st[now].t0.tv_sec-st[prev].t0.tv_sec;
                        temp.tv_nsec = st[now].t0.tv_nsec-st[prev].t0.tv_nsec;
                    }
                    //n = st[now].t0.tv_nsec - st[prev].t0.tv_nsec;
                    n = temp.tv_nsec;
                    m = n / 1000000.0;
                    tm += m;
                }
                tf += st[now].fc;
            }
            printf("%c [%d] %d sec, %d nsec, %d cbc, %d fc",
                c,
                i,
                st[i].t0.tv_sec - starter.tv_sec,
                st[i].t0.tv_nsec,
                st[i].cbc,
                st[i].fc);
            if (n) {
                printf(", (%d nsec / %f msec)\n", n, m);
            } else {
                puts("");
            }
        }
        printf("tm = %f / tf = %d\n", tm, tf);
        //sleep(1);
        //usleep(1000000);
        usleep(250000);
    }
    return 0;
}
