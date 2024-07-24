#include <cstdio>
#include <csignal>
#include "alsa_read_write.h"
#include "alsa.h"
#include "usb_camer_audio.h"


static bool quit = false;

static void sigterm_handler(int sig) {
    fprintf(stderr, "******************signal %d, SIGUSR1 = %d\n", sig, SIGUSR1);
    quit = true;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);
    signal(SIGUSR1, sigterm_handler);

    init_alsa();

    // 仅仅用于发于usb camera带的声卡
    init_usb_camera();


    while (!quit) {
        usleep(10*1000);
    }

    printf("--------------------------->>>>>>>> deinit! %d\n", __LINE__);

    deinit_usb_camera();
    printf("--------------------------->>>>>>>> deinit! %d\n", __LINE__);
    deinit_alsa();

    printf("debug quit over!!\n");

    return 0;
}
