#pragma once

#include "MicroBit.h"
// not yet needed outside -> stay private
//void radio_disable(void);
//uint8_t channel_to_freq(int channel);

void radio_init(void);

void radio_send(uint32_t accessAddress, int channel, uint8_t* tx_buffer, int size);
void radio_receive(uint32_t accessAddress, int channel, uint8_t* rx_buffer);
void inquiry(void);