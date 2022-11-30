#include "radio.h"
#include "packet.h"

// TEMP TCPIP stuff
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/file.h> 
#include <fcntl.h>
#include <unistd.h>

namespace Telemetry
{

// TEMP TCPIP STUFF
int servfd = -1;
int sockfd = -1;

//void txPacket(unsigned char* data, int len)
//{
    // RADIO MODULE CODE GOES HERE
    // CURRENTLY TCP/IP IS BEING USED FOR TESTING
//}

void txPacket(unsigned char* data, int len)
{
    int result = write(sockfd, data, len);
    if(result < 0) {
        spdlog::error("Failed to send packet!");
        return;
    }
}

void txSensors(sensor_data* sensors)
{
    if(servfd < 0)
        initIP();

    unsigned char* data = new unsigned char[32];
    formatPacket(sensors, &data);
    txPacket(data, 32);
    delete data;
}

// This will be removed after initial development is done
// Just exists for testing right now
void initIP()
{
    servfd = socket(AF_INET, SOCK_STREAM, 0);
    if(servfd < 0) {
        spdlog::error("Failed to open socket!");
        return;
    }

    struct sockaddr_in addr;
    int opt = -1;
    int addrlen = sizeof(addr);
    if(setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { // could fail here
        spdlog::error("Failed to set socket options");
        return;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(5555);

    if(bind(servfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        spdlog::error("Failed to bind socket");
        return;
    }
    spdlog::info("Listening for connection...");
    if(listen(servfd, 1) < 0) {
        spdlog::error("Failed to listen on socket");
        return;
    }   
    if((sockfd = accept(servfd, (struct sockaddr*)&addr, (socklen_t*)&addrlen)) < 0) {
        spdlog::error("Failed to accept incoming connection");
        return;
    }
    spdlog::info("Accepted connection");
}

}