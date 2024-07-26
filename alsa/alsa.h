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

void new_get_meter(void *_meter);
void new_set_hdmi_in_volume(float value);
void new_set_line_in_volume(float value);
void new_set_usb_in_volume(float value);
void new_set_mp4_in_volume(float value);
void new_set_line_out_volume(float value);

void new_set_hdmi_in_enable(int value);
void new_set_line_in_enable(int value);
void new_set_usb_in_enable(int value);
void new_set_mp4_in_enable(int value);
void new_set_line_out_enable(int value);

void new_set_hdmi_audio_out_enable(int value);
void new_usb_audio_out_enable(int value);
void new_35_audio_out_enable(int value);

int new_mp4_audio_write(void *data, int size);
#endif //RK_TEST_ALSA_H
