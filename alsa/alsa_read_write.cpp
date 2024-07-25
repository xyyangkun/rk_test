#include <cstdio>
#include <csignal>
#include <thread>
#include "alsa_read_write.h"
#include "alsa.h"
#include "usb_camer_audio.h"
#include "rkav_interface.h"


static bool quit = false;

static void sigterm_handler(int sig) {
    fprintf(stderr, "******************signal %d, SIGUSR1 = %d\n", sig, SIGUSR1);
    quit = true;
}

bool proc_get_vu_pk_quit = false;
static t_meter _meter;
void *proc_get_vu_pk(void *parm)
{
    while(!proc_get_vu_pk_quit)
    {
        new_get_meter((void*)&_meter);
        t_meter_one *one = &_meter.hdmi_in;
        // printf("vu_l:%d vu_r:%d pk_l:%d pk_r:%d\n", one->vu_left, one->vu_right, one->pk_left, one->pk_right);
        usleep(67*1000);
    }
    return NULL;
}

int uac_write(void *buf, unsigned int size) {
    // printf("uac write size:%d\n", size);
    return 0;
}

int aac_encode_write(void* buf, unsigned int size) {
    // printf("aac write size:%d\n", size);
    return 0;
}
int main(int argc, char *argv[])
{
    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);
    signal(SIGUSR1, sigterm_handler);

    init_alsa();

    // 仅仅用于发于usb camera带的声卡
    init_usb_camera();

    // 创建获取vu_pk线程
    pthread_t vu_pk_th;
    proc_get_vu_pk_quit = false;
    pthread_create(&vu_pk_th, NULL, proc_get_vu_pk, NULL);

    while (!quit) {
        usleep(10*1000);
    }

    proc_get_vu_pk_quit = true;
    pthread_join(vu_pk_th, NULL);

    printf("--------------------------->>>>>>>> deinit! %d\n", __LINE__);

    deinit_usb_camera();
    printf("--------------------------->>>>>>>> deinit! %d\n", __LINE__);
    deinit_alsa();

    printf("debug quit over!!\n");

    return 0;
}
