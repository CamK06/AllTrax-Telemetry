#include "version.h"
#include "alltrax/alltrax.h"
#include "link.h"
#include "packet.h"
#include "util.h"
#include "gps.h"

#include <flog.h>
#include <signal.h>
#include <cstring>
#include <vector>

#define SERIAL_PORT "/dev/ttyUSB1"

int monCalls = 0;
unsigned char* outData = nullptr;
bool transmitting = false;

void radio_callback(unsigned char* data, int len, TLink::DataType type)
{
	if(type != TLink::DataType::Command)
		return;

	// Decode the command
	if(memcmp(data, "TOGGLE", len) == 0)
		transmitting = !transmitting;
}

void monitor_callback(sensor_data* sensors)
{
	// Don't transmit unless the receiver has told us to
	if(!transmitting)
		return;

	// Get GPS position or fake it
	gps_pos* pos = (gps_pos*)malloc(sizeof(gps_pos));
#ifndef USE_FAKE_GPS
	pos = GPS::getPosition();
#else 
 	pos->latitude = sin(random())*85;
 	pos->longitude = cos(random())*80;
 	pos->velocity = fabs((sin(random()*10)*cos(random()*20))*35);
#endif

	// If we don't have any data, create a new buffer by copying the first packet PKT_BURST times
	if(outData == nullptr) {
		outData = new unsigned char[PKT_BURST*PKT_LEN];
		unsigned char* data = new unsigned char[PKT_LEN];
		Telemetry::formatPacket(sensors, pos, &data);
		for(int i = 0; i < PKT_BURST; i++)
			memcpy(outData+(i*PKT_LEN), data, PKT_LEN);
		delete data;
	}
	else {
		// Shift all packets up to give room for the new packet
		unsigned char* tmp = new unsigned char[PKT_BURST*PKT_LEN];
		memcpy(tmp+PKT_LEN, outData, (PKT_BURST-1)*PKT_LEN);
		memcpy(outData, tmp, PKT_BURST*PKT_LEN);
		delete tmp;

		// Add new packet to beginning
		Telemetry::formatPacket(sensors, pos, &outData);
	}

	// Send past packet burst every 3 seconds
	if(++monCalls == TX_RATE) {
		Util::dumpHex(outData, PKT_BURST*PKT_LEN);
		TLink::sendData(outData, PKT_BURST*PKT_LEN, TLink::DataType::Sensor, false);
		monCalls = 0;
	}
	delete pos;
}

int main()
{
	flog::info("AllTrax SR Telemetry TX " VERSION);
	flog::info("HIDAPI " HID_API_VERSION_STR);

	TLink::init((char*)SERIAL_PORT);
	TLink::setRxCallback(&radio_callback);
#ifndef USE_FAKE_GPS
	GPS::init();
#endif

	Alltrax::setMonitorCallback(&monitor_callback);
#ifndef USE_FAKE_CONTROLLER
	if(!Alltrax::initMotorController())
#else
	if(!Alltrax::initMotorController(true))
#endif
		return -1;
	Alltrax::startMonitor(PKT_RATE);
	while(Alltrax::monThreadRunning);

	Alltrax::cleanup();
	TLink::cleanup();
#ifndef USE_FAKE_GPS
	GPS::close();
#endif
	flog::info("Exiting...");
	return 0;
}
