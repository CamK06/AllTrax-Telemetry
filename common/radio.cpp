#include "radio.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/file.h> 
#include <sys/signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <flog.h>
#include <cstring>

namespace Radio
{

int radiofd = -1;
char* serialPort = nullptr;
radio_rx_callback_t rxCB = nullptr;
int txErrors = 0;
bool isClient = false;

int sendData(unsigned char* data, int len)
{
    // Exit if the fd isn't set/radio isn't open
    if(radiofd < 0) {
        flog::error("Failed to send packet! Radio is not open.");
        return -1;
    }

    // Write the data to the radio socket
    if(write(radiofd, data, len) < 0) {
        flog::error("Failed to send packet!");
        txErrors++;

        // If we've failed to send more than 5 times, we've lost connection,
        // listen for another.
        if(txErrors >= 3) {
            tcflush(radiofd, TCIOFLUSH);
            ::close(radiofd);
            radiofd = -1;
            txErrors = 0;
            flog::error("Too many packets failed to send; giving up.");
            init(serialPort, rxCB, isClient);
        }
        return -1;
    }
    flog::debug("Successfully sent {} bytes", len);
    return 0;
}

void init(const char* port, radio_rx_callback_t rxCallback, bool client)
{
	serialPort = (char*)port;
    isClient = client;
    rxCB = rxCallback;

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
    }
#else
    // Open the serial port
    flog::info("Opening {}...", serialPort);
    radiofd = open(serialPort, O_RDWR | O_NOCTTY | O_NDELAY); // O_SYNC was here
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
    tty.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
    tty.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | OFILL | OPOST);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    tty.c_cflag &= ~(CSIZE | PARENB);
    tty.c_cflag |= CS8;

    // Apply options
    if(tcsetattr(radiofd, TCSAFLUSH, &tty) != 0) {
        flog::error("Failed to set serial port attributes");
        return;
    }
#endif
    
    // Set up the receive signal/callback
    
    // Set port to non-blocking RX
    int flags = fcntl(radiofd, F_GETFL, 0);
    fcntl(radiofd, F_SETFL, flags | O_NONBLOCK | O_ASYNC);

    signal(SIGIO, rxHandler);
    if(fcntl(radiofd, F_SETOWN, getpid()) < 0) {
        flog::error("Error setting sigio handler");
        return;
    }
    tcflush(radiofd, TCIOFLUSH);

    flog::info("Opened port {}", serialPort);
}

void rxHandler(int sigrxCallback)
{
    if(rxCB == nullptr) {
        flog::error("RX callback is not set!");
        return;
    }

    rxCB(radiofd);
}

void close() { ::close(radiofd); }

}