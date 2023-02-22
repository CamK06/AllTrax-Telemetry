#include "radio.h"
#include "version.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/file.h> 
#include <sys/signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <spdlog/spdlog.h>

namespace Radio
{

// Serial port
int radiofd = -1;
char* serialPort = nullptr;

// TX/RX stats
int rxPackets = 0;
int txErrors = 0;
int rxErrors = 0;

bool isClient = false;
radio_rx_callback_t rxCallback = nullptr;

// This can probably be deleted as we're doing bulk packet sending now
void sendSensors(sensor_data* sensors, gps_pos* gps)
{
    // Format and send a packet with the sensor data
    unsigned char* packet = new unsigned char[64];
    Telemetry::formatPacket(sensors, gps, &packet);
    sendData(packet, 64);
    delete packet;
}

void sendData(unsigned char* data, int len)
{
    // Exit if the fd isn't set/radio isn't open
    if(radiofd < 0) {
        spdlog::error("Failed to send packet! Radio is not open.");
        return;
    }

    // Write the data to the radio socket
    if(write(radiofd, data, len) < 0) {
        spdlog::error("Failed to send packet!");
        txErrors++;

        // If we've failed to send more than 5 times, we've lost connection,
        // listen for another.
        if(txErrors >= 3) {
            ::close(radiofd);
            radiofd = -1;
	        rxErrors = 0;
	        rxPackets = 0;
            txErrors = 0;
            spdlog::error("Too many packets failed to send; giving up.");
            init(serialPort, isClient);
        }
        return;
    }
    spdlog::debug("Successfully sent {} bytes", len);
}

void receiveData(int sig)
{
    // Wait for 0.1s before continuing to read, to allow all
    // bytes to come through the port before reading
    usleep(1000000);
    unsigned char* packet = new unsigned char[PKT_LEN*PKT_BURST];

    // Read PKT_LEN*PKT_BURST bytes from the radio
    int nr = read(radiofd, packet, PKT_LEN*PKT_BURST);
    int packetsToDecode = PKT_BURST;
    if(nr < 0) {
        spdlog::debug("Double read attempted!");
        return;
    }
    else if(nr != PKT_LEN*PKT_BURST && nr < PKT_LEN) {
        spdlog::warn("Received {} bytes, expected {}", nr, PKT_LEN*PKT_BURST);
        rxErrors++;
        return;
        //if(nr < (PKT_LEN*PKT_BURST)-2)
        //    return;
    }
    if(nr != PKT_LEN*PKT_BURST && nr > PKT_LEN)
        packetsToDecode = 1;
    rxPackets++;

    // Print RX statistics, this may be removed later
    spdlog::info("Received {} bytes", nr);
    spdlog::info("Non-critical RX errors: {}", rxErrors);
    spdlog::info("RX percent error: {0:.1f}%", (rxErrors*1.0f/(rxErrors+rxPackets)) * 100.0f);

    // Decode the packets
    for(int i = 0; i < packetsToDecode; i++) {
        sensor_data sensors = sensor_data();
        gps_pos gps = gps_pos();
        time_t timestamp = 0;
        Telemetry::decodePacket(packet+(i*PKT_LEN), &sensors, &gps, &timestamp);
        
        // Send decoded packet to RX callback
        if(rxCallback != nullptr)
            rxCallback(sensors, gps, timestamp);
    }
    
    // Cleanup; delete the rx buffer and flush the serial port
    delete packet;
    tcflush(radiofd, TCIFLUSH);
}

void init(const char* port, bool client)
{
	serialPort = (char*)port;
    isClient = client;

#ifdef USE_IP
    radiofd = socket(AF_INET, SOCK_STREAM, 0);
    if(radiofd < 0) {
        spdlog::error("Failed to open socket");
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
            spdlog::error("Failed to open socket");
            return;
        }

        if(setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            spdlog::error("Failed to set socket options");
            return;
        }
        addr.sin_addr.s_addr = INADDR_ANY;
        if(bind(servfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            spdlog::error("Failed to bind socket");
            return;
        }

        spdlog::info("Listening for connections...");
        if(listen(servfd, 1) < 0) {
            spdlog::error("Failed to listen on socket");
            return;
        }
        if((radiofd = accept(servfd, (struct sockaddr*)&addr, (socklen_t*)&addrlen)) < 0) {
            spdlog::error("Failed to accept incoming connection");
            return;
        }
        spdlog::info("Accepted connection");
    }
    else {
        if(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
            spdlog::error("Invalid IP address");
            return;
        }
        if(connect(radiofd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            spdlog::error("Failed connecting to server");
            return;
        }
    }
#else
    // Open the serial port
    spdlog::info("Opening {}...", serialPort);
    radiofd = open(serialPort, O_RDWR | O_NOCTTY | O_SYNC);
    if(radiofd < 0) {
        spdlog::error("Failed to open serial port");
        return;
    }

    // Set baud rate to 57600baud
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if(tcgetattr(radiofd, &tty) != 0) {
        spdlog::error("Failed to get serial port attributes");
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
        spdlog::error("Failed to set serial port attributes");
        return;
    }
#endif
    
    // Set up the receive signal/callback
    // This MIGHT need to be disabled in the tx program
    //if(isClient) {
    // Set port to non-blocking RX
    int flags = fcntl(radiofd, F_GETFL, 0);
    fcntl(radiofd, F_SETFL, flags | O_NONBLOCK | O_ASYNC);

    signal(SIGIO, receiveData);
    if(fcntl(radiofd, F_SETOWN, getpid()) < 0) {
        spdlog::error("Error setting sigio handler");
        return;
    }
    //}

    spdlog::info("Opened port {}", serialPort);
}

void close()
{
    ::close(radiofd);
}

void setRxCallback(radio_rx_callback_t callback)
{
    rxCallback = callback;
}

}
