#pragma once
#include <stdint.h>

// New packet protocol; AX.25 based:
//
// Frame format; second row is nr bytes:
// [flag][CarID][ACK][DataType][Bytes][Data][FCS][flag]
// [ 1  ][  1  ][ 1 ][   1    ][  1  ][<256][ 4 ][  1 ]
//
// - FCS will be calculated same as AX.25; CRC32
// - Checksum is CarID-Data, does NOT include flags
// - Flag is the same as AX.25
// - ACK field is either true or false; whether or not an ACK is requested from receiver
// - Max data bytes is 256
// - Car ID is just number of the car
// - BURST packets will likely NOT be supported
// 
// A frame is to be considered invalid if any of the following are true:
// - Less than 9 bytes were received
// - Either of the two flags are missing
// - The frame size is not divisible by 8** might not use this
//
// Data Types:
// 1 - Car telemetry
// 2 - Computer telemetry
// 3 - Command
// 4 - Command response
//
// Commands: *this might not be implemented
// CHSR [RATE] - Change data sample rate 
// CHTR [RATE] - Change data tx rate
// START - Start telemetry transmit loop
// STOP - Stop transmitting telemetry
// EXEC [CMD] - Execute a shell command

#define MAX_PKT_LEN 266
#define FLAG 0x7E

namespace TLink
{

extern int framesLost;

enum DataType
{
    Sensor, Computer, Command, Response
};

typedef void (*link_rx_callback_t)(unsigned char* data, int len, DataType dataType);

struct Frame
{
    uint8_t carID;
    uint8_t type;
    uint8_t numBytes;
    bool needsAck;
    unsigned char* data;
};

void init(char* port, bool client = false);
void radioRxCallback(int radiofd);
int sendData(unsigned char* data, int len, DataType dataType, bool requireAck);
void setCarID(uint8_t id);
void setRxCallback(link_rx_callback_t cb);
void cleanup();

// Lower level functions; not used directly by client

int sendFrame(Frame* frame); // Returns 0 on success, -1 on failure
int formatFrame(Frame* frame, unsigned char** outData); // Returns number of bytes in the frame
int decodeFrame(unsigned char* data, int len, Frame* outFrame); // Returns 0 on success, -1 on failure

}