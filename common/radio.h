#pragma once

#include "version.h"
#include "link.h"

typedef void (*radio_rx_callback_t)(int radiofd);

namespace Radio
{

int sendData(unsigned char* data, int len);
void rxHandler(int sig);
void close();
void init(const char* port, radio_rx_callback_t rxCallback, bool client = false);

}