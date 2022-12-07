#include "radio.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/file.h> 
#include <sys/signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

namespace Radio
{

// File descriptor to read/write to/from
int radiofd = -1;

#ifdef GUI_RX
MainWindow* mainWindowPtr = nullptr;
#else
radio_rx_callback_t rxCallback = nullptr;
#endif

void sendSensors(sensor_data* sensors)
{
    // Format and send a packet with the sensor data
    unsigned char* packet = new unsigned char[32];
    Telemetry::formatPacket(sensors, &packet);
    sendData(packet, 32);
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
        return;
    }
    spdlog::debug("Successfully sent {} bytes", len);
}

void receiveData(int sig)
{
    // Read 32 bytes from the radio
    unsigned char* packet = new unsigned char[32];
    if(read(radiofd, packet, 32) < 0) {
        spdlog::error("Failed to read packet!");
        return;
    }

    // Decode the packet
    sensor_data sensors = sensor_data();
    Telemetry::decodePacket(packet, &sensors);

    // Trigger the rx callback
#ifdef GUI_RX
    if(mainWindowPtr == nullptr)
        return;
    QMetaObject::invokeMethod(mainWindowPtr, "packetCallback", Qt::QueuedConnection, Q_ARG(sensor_data, sensors));
#else
    if(rxCallback != nullptr)
        rxCallback(packet);
#endif
}

#ifdef GUI_RX
void init(MainWindow* mainWindow)
{
    mainWindowPtr = mainWindow;
#else
void init()
{
#endif

    // TEMPORARY CODE BEGINS HERE; REPLACE WITH SERIAL
    radiofd = socket(AF_INET, SOCK_STREAM, 0);
    if(radiofd < 0) {
        spdlog::error("Failed to open socket");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5555);
#ifndef GUI_RX
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
#else
    if(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        spdlog::error("Invalid IP address");
        return;
    }
    if(connect(radiofd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        spdlog::error("Failed connecting to server");
        return;
    }
#endif

    // THIS WILL EXIST FOR SERIAL CODE; NON-TEMPORARY
    int flags = fcntl(radiofd, F_GETFL, 0);
    fcntl(radiofd, F_SETFL, flags | O_NONBLOCK | O_ASYNC);
    
    // Set up the receive signal/callback
    signal(SIGIO, receiveData);
    if(fcntl(radiofd, F_SETOWN, getpid()) < 0) {
        spdlog::error("Error setting sigio handler");
        return;
    }
}

}