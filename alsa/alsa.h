//
// Created by win10 on 2024/7/15.
//

#ifndef RK_TEST_ALSA_H
#define RK_TEST_ALSA_H

#define AUDIO_FRAME_SIZE 480
#define AUDIO_READ_CHN  2  // 2 4
#define AUDIO_WRITE_CHN 2  // 4 6

int init_alsa();
int deinit_alsa();
#endif //RK_TEST_ALSA_H
