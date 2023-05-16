#include "link.h"
#include "version.h"
#include <string.h>
#include <zlib.h>
#include <fstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/file.h> 
#include <sys/signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <chrono>
#include <thread> 
#include "flog.h"

namespace TLink {

// Radio
int radiofd = -1;
char* serialPort = nullptr;
unsigned char incomingData[2048];
int alreadyRead = 0;

// Link
Status status = Status::Disconnected;
bool isClient; // Client is the car, host is the base
bool hasAck = false; // Whether or not an ACK was received, should be reset after use
uint8_t carID = 0;
link_rx_callback_t rxCB = nullptr;
int fails = 0;
int rxFails = 0;

void init(char* port, bool client)
{
    // Open the serial port
    serialPort = (char*)port;
    isClient = client;

#ifdef USE_IP
    radiofd = socket(AF_INET, SOCK_STREAM, 0);
    if(radiofd < 0) {
        flog::error("Failed to open socket");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5555);
    if(!client) {
        int servfd = -1;
        int opt = -1;
        int addrlen = sizeof(addr);

        if((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            flog::error("Failed to open socket");
            return;
        }

        if(setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            flog::error("Failed to set socket options");
            return;
        }
        addr.sin_addr.s_addr = INADDR_ANY;
        if(bind(servfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            flog::error("Failed to bind socket");
            return;
        }

        flog::info("Listening for connections...");
        if(listen(servfd, 1) < 0) {
            flog::error("Failed to listen on socket");
            return;
        }
        if((radiofd = accept(servfd, (struct sockaddr*)&addr, (socklen_t*)&addrlen)) < 0) {
            flog::error("Failed to accept incoming connection");
            return;
        }
        flog::info("Accepted connection");
    }
    else {
        if(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
            flog::error("Invalid IP address");
            return;
        }
        if(connect(radiofd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            flog::error("Failed connecting to server");
            return;
        }
	flog::info("Connected");
    }
#else
    // Open the serial port
    flog::info("Opening {}...", serialPort);
    radiofd = open(serialPort, O_RDWR | O_NOCTTY | O_SYNC);
    if(radiofd < 0) {
        flog::error("Failed to open serial port");
        return;
    }

    // Set baud rate to 57600baud
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if(tcgetattr(radiofd, &tty) != 0) {
        flog::error("Failed to get serial port attributes");
        return;
    }
    cfsetispeed(&tty, B57600);
    cfsetospeed(&tty, B57600);

    // Set port options; 8 data bits 1 stop bit
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~IEXTEN;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE; 
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    // Apply options
    if(tcsetattr(radiofd, TCSANOW, &tty) != 0) {
        flog::error("Failed to set serial port attributes");
        return;
    }
#endif
    
    // Set port to non-blocking RX
    int flags = fcntl(radiofd, F_GETFL, 0);
    fcntl(radiofd, F_SETFL, flags | O_NONBLOCK | O_ASYNC);
    
    // Set up the receive signal/callback
    signal(SIGIO, radioRxCallback);
    if(fcntl(radiofd, F_SETOWN, getpid()) < 0) {
        flog::error("Error setting sigio handler");
        return;
    }
    flog::info("Opened port {}", serialPort);

    initConnection();
}

void initConnection()
{
    // Flush the serial buffers
    tcflush(radiofd, TCIOFLUSH);

    // Reset the RX buffers
    // TODO: Implement

    // Reset link info
    status = Status::Disconnected;
    fails = 0;

    // Handshake with the other radio (wait until it's available)
    if(isClient) // Car doesn't send pings
        return;
    
    // Periodically send pings until we get a response
    flog::info("Finding client...");
    while(sendSig(Signal::CON, true) < 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send an ACK, completing the handshake
    sendSig(Signal::ACK);

    flog::info("Connected!");
    status = Status::Connected;
}

void radioRxCallback(int sig)
{
    usleep(100000);

    // Read the serial buffer into data[]
    unsigned char data[MAX_PKT_LEN];
    int len = read(radiofd, data, MAX_PKT_LEN);
    if(len < 0) {
        flog::error("Failed to read from radio!");
	    alreadyRead = 0;
        return;
    }

    if(alreadyRead+len >= 2048) { // The buffer should NEVER get this full, something's wrong
        alreadyRead = 0;
	    tcflush(radiofd, TCIFLUSH);
	    rxFails++;
	    flog::error("RX buffer overflowed");
	    return;
    }
    memcpy(incomingData+alreadyRead, data, len);
    alreadyRead += len;

    // Verify the incoming frame by testing its start/stop flags
    if(incomingData[0] != FLAG) {
        flog::error("Invalid frame! No start flag");
        alreadyRead = 0;
	    tcflush(radiofd, TCIFLUSH);
        //rxFails++;
	return;
    }
    if(incomingData[alreadyRead-1] != FLAG) {
        flog::error("Invalid frame! No stop flag");
       	alreadyRead = 0;
       	tcflush(radiofd, TCIFLUSH);
        //rxFails++;
	return;
    }

    // Decode the frame
    Frame frame;
    if(decodeFrame(incomingData, alreadyRead, &frame) < 0) {
        flog::error("Failed to decode frame!");
        rxFails++;
	    alreadyRead = 0;
	    return;
    }
    alreadyRead = 0;

    // Send an ACK if the frame asked for one
    if(frame.needsAck && frame.type != DataType::Signal)
        sendSig(Signal::ACK);

    // Check if the frame was an ACK signal
    if(frame.type == DataType::Signal && frame.data[0] == Signal::ACK) {
        hasAck = true;

        // Handshake is complete; this was the last ACK
        if(status == Status::Handshaking) {
            hasAck = false;
            initConnection();
            status = Status::Connected;
            flog::info("Connected!");
        }
        return;
    }

    // Check if the frame was a CON signal
    if(frame.type == DataType::Signal && frame.data[0] == Signal::CON) {
        if(sendSig(Signal::ACK) < 0) {
		    flog::error("Failed to ACK connection request!");
		    return;
	    }
	    hasAck = false;
	    status = Status::Handshaking;
	    return;
    }

    // Check if the frame was a DSC signal
    if(frame.type == DataType::Signal && frame.data[0] == Signal::DSC) {
        if(sendSig(Signal::ACK) < 0)
            flog::error("Failed to ACK disconnect request!");
        hasAck = false;
        status = Status::Disconnected;
        //tcflush(radiofd, TCIFLUSH);
    }

    // Call the frame RX callback
    if(rxCB != nullptr && frame.numBytes > 1)
    	rxCB(frame.data, frame.numBytes, frame.type);

    // Cleanup
    alreadyRead = 0;
    //tcflush(radiofd, TCIFLUSH);
}

int sendData(unsigned char* data, int len, DataType dataType, bool requireAck)
{
    // TODO: Split data into multiple frames if longer than MAX_PKT_LEN-10
    if(len > MAX_PKT_LEN-10) {
        flog::error("Data too long to send!");
        return -1;
    }

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
    int timer = 0;
    //while(!hasAck) {
        if(radiofd < 0) {
            flog::error("Radio is not open!");
            return -1;
        }
        if(write(radiofd, outData, frameLen) < 0) {
            flog::error("Failed to send packet!");
            // TODO: Add a threshold to disconnect
            return -1;
        }
    while(!hasAck) {
        // Don't wait for an ACK we don't want
        if(!requireAck)
            break;

        // If there's no ACK after 5 seconds, we failed
        timer++;
        if(timer > 5) {
            fails++;
            return -1;
        }
        sleep(1);
    }

    // Cleanup
    delete[] outData;
    if(requireAck) // Don't interfere if something else wants an Ack
        hasAck = false;

    // If we've failed to send 5 times, we're disconnected
    if(fails > 5)
            initConnection();
    return 0;
}

// Lower level functions; not used outside of this file

int sendSig(int signal, bool needsAck)
{
    // Send the signal
    if(sendData((unsigned char*)&signal, 1, DataType::Signal, needsAck) < 0)
        return -1;
    else
        return 0;
}

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
    // Dump the frame hex
    flog::debug("Frame: ");
    for(int i = 0; i < len; i++) {
        if(i % 16 == 0 && i != 0)
            printf("\n");
        printf("%02X ", data[i]);
    }
    printf("\n");
    flog::debug("Length: {}", len);

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

void setCarID(uint8_t id) { carID = id; }
void setRxCallback(link_rx_callback_t cb) { rxCB = cb; }

}
