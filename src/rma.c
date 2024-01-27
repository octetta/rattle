// libminiaudio-audio.c
// functions for running AMY on a computer
#if !defined(ESP_PLATFORM) && !defined(PICO_ON_DEVICE) && !defined(ARDUINO)
#include "amy.h"

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

#include "miniaudio.h"

#include <stdio.h>
#include <unistd.h>

#define DEVICE_FORMAT       ma_format_s16

int16_t * leftover_buf;
uint16_t leftover_samples = 0;
int16_t amy_device_id = -1;
uint8_t amy_running = 0;
pthread_t amy_live_thread;


void amy_print_devices() {
    ma_context context;
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        fprintf(stdout,"Failed to setup context for device list.\n");
        exit(1);
    }

    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
        fprintf(stdout,"[[-1, \"Failed to get device list.\"]]\n");
        exit(1);
    }

    char c = ',';
    fprintf(stdout, "[");
    for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
        if (iDevice == playbackCount -1) c = ']';
        fprintf(stdout,"[%d, \"%s\"]%c\n",
            iDevice, pPlaybackInfos[iDevice].name, c);
    }

    ma_context_uninit(&context);
}

static short *capture_buf = NULL;
static int capture_active = 0;
static unsigned int capture_req_frames = 0;
static unsigned int capture_rcv_frames = 0;

void capture_start(short *buf, unsigned int frames, short *channels) {
    if (channels) *channels = AMY_NCHANS;
    capture_active = 0;
    capture_buf = buf;
    if (frames > 0) {
        capture_rcv_frames = 0;
        capture_req_frames = frames;
        capture_active = 1;
    }
}

unsigned int captured_frames(void) {
    return capture_rcv_frames;
}

void capture_stop(void) {
    capture_active = 0;
}

unsigned int cb_frame_count = 0;

void capture(short n) {
    if (capture_active) {
        capture_buf[capture_rcv_frames++] = n;
        if (capture_rcv_frames >= capture_req_frames) capture_active = 0;
    }
}

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frame_count) {
    // Different audio devices on mac have wildly different frame_count_maxes, so we have to be ok with 
    // an audio buffer that is not an even multiple of BLOCK_SIZE. my iMac's speakers were always 512 frames, but
    // external headphones on a MBP is 432 or 431, and airpods were something like 1440.

    //printf("AMY_BLOCK_SIZE:%d frame_count:%d leftover_samples:%d\n", AMY_BLOCK_SIZE, frame_count, leftover_samples);

    cb_frame_count = frame_count;

    short int *poke = (short *)pOutput;

    // First send over the leftover samples, if any
    int ptr = 0;

    for(uint16_t frame=0;frame<leftover_samples;frame++) {
        for(uint8_t c=0;c<pDevice->playback.channels;c++) {
            short n = leftover_buf[AMY_NCHANS * frame + c];
            poke[ptr++] = n;
            capture(n);
        }
    }

    frame_count -= leftover_samples;
    leftover_samples = 0;

    // Now send the bulk of the frames
    for(uint8_t i=0;i<(uint8_t)(frame_count / AMY_BLOCK_SIZE);i++) {
        int16_t * buf = amy_simple_fill_buffer();
        for(uint16_t frame=0;frame<AMY_BLOCK_SIZE;frame++) {
            for(uint8_t c=0;c<pDevice->playback.channels;c++) {
                short n = buf[AMY_NCHANS * frame + c];
                poke[ptr++] = n;
                capture(n);
            }
        }
    } 

    // If any leftover, let's put those in the outgoing buf and the rest in leftover_samples
    uint16_t still_need = frame_count % AMY_BLOCK_SIZE;
    if(still_need != 0) {
        int16_t * buf = amy_simple_fill_buffer();
        for(uint16_t frame=0;frame<still_need;frame++) {
            for(uint8_t c=0;c<pDevice->playback.channels;c++) {
                short n = buf[AMY_NCHANS * frame + c];
                poke[ptr++] = n;
                capture(n);
            }
        }
        memcpy(leftover_buf, buf+(still_need*AMY_NCHANS), (AMY_BLOCK_SIZE - still_need)*2*AMY_NCHANS);
        leftover_samples = AMY_BLOCK_SIZE - still_need;
    }
}

ma_device_config deviceConfig;
ma_device device;
unsigned char _custom[4096];
ma_context context;
ma_device_info* pPlaybackInfos;
ma_uint32 playbackCount;
ma_device_info* pCaptureInfos;
ma_uint32 captureCount;

// start 

amy_err_t miniaudio_init() {
    leftover_buf = malloc_caps(sizeof(int16_t)*AMY_BLOCK_SIZE*AMY_NCHANS, FBL_RAM_CAPS);
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

void *miniaudio_run(void *vargp) {
    miniaudio_init();
    while(amy_running) {
        sleep(1);
    }
    return NULL;
}

void amy_live_start() {
    // kick off a thread running miniaudio_run
    amy_running = 1;
    pthread_create(&amy_live_thread, NULL, miniaudio_run, NULL);
}


void amy_live_stop() {
    amy_running = 0;
    ma_device_uninit(&device);
    free(leftover_buf);
}
#endif
