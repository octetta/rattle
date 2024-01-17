#include <stdio.h>
#include "amy.h"
int main(int argc, char *argv[]) {
    amy_print_devices();
    amy_start(/* cores= */ 1, /* reverb= */ 0, /* chorus= */ 0);
    amy_live_start();
    amy_reset_oscs();
    amy_play_message("v0w8n50p30l1Z");
    printf("sleep 5\n");
    sleep(5);
    amy_live_stop();
    return 0;
}
