#include "radio.h"

// Eventually in the final program this should be a universal radio driver under common

// TEMP TCPIP stuff
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/file.h> 
#include <fcntl.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

int sockfd = -1;
MainWindow* mainWindowPtr = nullptr;

namespace Telemetry
{

void handle_sigio(int sig)
{
    unsigned char buffer[32];
    int n = read(sockfd, buffer, 32);
    if(n < 0) {
        spdlog::error("Failed to read bytes from socket");
        return;
    }
    spdlog::debug("Received {} bytes from server", n);

    // Decode the packet
    sensor_data sensors = sensor_data();
    Telemetry::decodePacket(buffer, &sensors);

    if(mainWindowPtr == nullptr)
        return;
    QMetaObject::invokeMethod(mainWindowPtr, "packetCallback", Qt::QueuedConnection, Q_ARG(sensor_data, sensors));
}

void initRadio(MainWindow* mainWindow)
{
    mainWindowPtr = mainWindow;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        spdlog::error("Failed to open socket");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5555);
    if(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        spdlog::error("Invalid IP address");
        return;
    }

    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        spdlog::error("Failed connecting to server");
        return;
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK | O_ASYNC);

    signal(SIGIO, handle_sigio);
    if(fcntl(sockfd, F_SETOWN, getpid()) < 0) {
        spdlog::error("Error setting sigio handler");
        return;
    }
}

}