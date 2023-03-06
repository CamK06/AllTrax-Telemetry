#include "link.h"
#include "radio.h"
#include <string.h>
#include <termios.h>
#include <flog.h>
#include <zlib.h>

namespace TLink {

uint8_t carID = 0;
link_rx_callback_t rxCB = nullptr;
bool awaitingAck = false;
int alreadyRead = 0;
unsigned char* incomingData = nullptr;
int framesLost = 0;

void radioRxCallback(int radiofd)
{
    usleep(10000);

    // Read the data from the radio file descriptor
    unsigned char data[MAX_PKT_LEN];
    int len = read(radiofd, data, MAX_PKT_LEN);
    if(len < 0) {
        flog::error("Failed to read from radio!");
        return;
    }

    // Check for framing characters
    if(data[0] != FLAG && incomingData == nullptr) {
        flog::error("Invalid frame!");
        return;
    }

    // Store the data in incomingData for the next rxcallback call,
    // as we have verified there is a frame coming in (via the flag)
    if(incomingData == nullptr)
        incomingData = new unsigned char[MAX_PKT_LEN];
    memcpy(incomingData+alreadyRead, data, len);
    alreadyRead += len;

    // Check the number of bytes
    if(len < 4 || alreadyRead < incomingData[4]+10)
        return;

    // Check for stop flag, give up if not found
    if(incomingData[alreadyRead-1] != FLAG) {
        flog::error("Invalid frame!");
        alreadyRead = 0;
        delete[] incomingData;
        incomingData = nullptr;
        tcflush(radiofd, TCIOFLUSH);
        framesLost++;
        return;
    }

    // Decode the frame
    Frame frame;
    if(decodeFrame(incomingData, alreadyRead, &frame) < 0) {
        flog::error("Failed to decode frame!");
        framesLost++;
        return;
    }

    // If the frame requires an ack, send one
    if(frame.needsAck) {
        unsigned char ack = 0xff; // TODO: Come up with a better way to do this
        if(sendData(&ack, 1, DataType::Response, false) < 0) {
            flog::error("Failed to send ack!");
            return;
        }
    }
    bool wasAck = false;
    if(awaitingAck && frame.type == DataType::Response
    && frame.data[0] == 0xff) {
        wasAck = true;
        awaitingAck = false;
    }

    // Call the rx callback
    if(rxCB != nullptr && !wasAck)
        rxCB(frame.data, frame.numBytes, (DataType)frame.type);

    // Cleanup for next packet
    delete[] incomingData;
    incomingData = nullptr;
    alreadyRead = 0;
    tcflush(radiofd, TCIOFLUSH);
}

int sendData(unsigned char* data, int len, DataType dataType, bool requireAck)
{
    // Frame the data
    Frame frame;
    frame.carID = carID;
    frame.type = dataType;
    frame.numBytes = len;
    frame.needsAck = requireAck;
    frame.data = new unsigned char[len];
    memcpy(frame.data, data, len);

    // Format the frame
    unsigned char* outData = new unsigned char[10+len];
    int frameLen = formatFrame(&frame, &outData);
    if(frameLen < 0) {
        flog::error("Failed to format frame!");
        return -1;
    }

    // Send the frame
    awaitingAck = true;
    int attempts = 0;
    while(awaitingAck) {
        // Send the data and wait 1s for an ack
        if(Radio::sendData(outData, frameLen) < 0)
            return -1;
        awaitingAck = requireAck;
        sleep(1+attempts);

        // Check if an ACK was received in the past second
        if(!awaitingAck)
            break;
        
        // Don't try more than 3 times
        attempts++;
        if(attempts > 3) {
            flog::error("Failed to send packet after 3 attempts; No ACK received.");
            return -1;
        }
    }

    delete[] outData;
    delete[] incomingData;
    incomingData = nullptr;
    alreadyRead = 0;
    return 0;
}

void setCarID(uint8_t id) { carID = id; }
void setRxCallback(link_rx_callback_t cb) { rxCB = cb; }
void init(char* port, bool client) { Radio::init(port, radioRxCallback, client); }

// Lower level functions

int formatFrame(Frame* frame, unsigned char** outData)
{
    (*outData)[0] = FLAG;
    (*outData)[1] = frame->carID;
    (*outData)[2] = frame->needsAck ? 0xff : 0x00;
    (*outData)[3] = frame->type;
    (*outData)[4] = frame->numBytes;
    memcpy(((*outData)+5), frame->data, frame->numBytes);
    uint32_t crc = crc32(0, (const Bytef*)((*outData)+1), 4+frame->numBytes);
    memcpy(((*outData)+5+frame->numBytes), &crc, 4);
    (*outData)[9+frame->numBytes] = FLAG;
    return 10+frame->numBytes;
}

int decodeFrame(unsigned char* data, int len, Frame* outFrame)
{
    // Verify frame start and stop flags
    if(data[0] != FLAG || data[len-1] != FLAG) {
        flog::error("Invalid frame! Start/Stop flag missing.");
        return -1;
    }

    // Verify integrity of the frame with the FCS
    uint32_t crc = crc32(0, (const Bytef*)(data+1), len-6);
    uint32_t fcs;
    memcpy(&fcs, data+len-5, 4);
    if(crc != fcs) {
        flog::error("Invalid frame! FCS does not match.");
        return -1;
    }

    // Decode the frame data
    outFrame->carID = data[1];
    outFrame->type = data[3];
    outFrame->numBytes = data[4];
    outFrame->data = new unsigned char[outFrame->numBytes];
    memcpy(outFrame->data, data+5, outFrame->numBytes);
    outFrame->needsAck = data[2] == 0xff ? true : false;
    return 0;
}

void cleanup()
{
    Radio::close();
    delete[] incomingData;
}

}