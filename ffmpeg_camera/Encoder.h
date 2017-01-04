#ifndef ENCODER_H
#define ENCODER_H
#include <stdint.h>

int Encoer_open(int in_w, int in_h);
int Encode_one_frame(uint8_t* Y, uint8_t* U, uint8_t* V, uint64_t i);
int Encoer_close();

#endif