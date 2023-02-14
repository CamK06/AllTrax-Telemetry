#define USE_FAKE_CONTROLLER true

#include "alltrax/alltrax.h"
#include "packet.h"
#include "radio.h"
#include "version.h"
#include "util.h"
#include "gps.h"

#include <spdlog/spdlog.h>
#include <signal.h>
#include <vector>

std::vector<sensor_data*> sensorHistory;
std::vector<gps_pos*> gpsHistory;
int monCalls = 0;
unsigned char* outData = nullptr;

void printSensors(sensor_data* sensors)
{
	// Battery
	spdlog::debug("Battery Voltage: {0:.1f}V", sensors->battVolt);
	spdlog::debug("Battery Current: {0:.1f}A", sensors->battCur);
	
	// Misc
	spdlog::debug("Throttle: {}%", sensors->throttle);
	spdlog::debug("Controller Temp: {0:.1f}C", sensors->controlTemp);
	spdlog::debug("Battery Temp: {0:.1f}C",  sensors->battTemp);
	printf("\n");	
}

void monitor_callback(sensor_data* sensors)
{
	printSensors(sensors);
	
	// Get GPS position or fake it
	gps_pos* pos = (gps_pos*)malloc(sizeof(gps_pos));
#ifndef USE_FAKE_CONTROLLER
	pos = GPS::getPosition();
#else 
	pos->latitude = sin(random())*85;
	pos->longitude = cos(random())*80;
	pos->velocity = fabs((sin(random()*10)*cos(random()*20))*35);
#endif

	// Add packet to history, for later json exporting
	sensorHistory.push_back(sensors);
	gpsHistory.push_back(pos);

	// If we don't have any data, create a new buffer by copying the first packet 300 times
	if(outData == nullptr) {
		outData = new unsigned char[300*64]; // 5 minute worth of data
		unsigned char* data = new unsigned char[64];
		Telemetry::formatPacket(sensors, pos, &data);
		for(int i = 0; i < 300; i++)
			memcpy(outData+(i*64), data, 64);
		delete data;
	}
	else {
		// Shift all packets up 64 bytes
		unsigned char* tmp = new unsigned char[300*64];
		memcpy(tmp+64, outData, 299*64);
		memcpy(outData, tmp, 300*64);
		delete tmp;

		// Add new packet to beginning
		Telemetry::formatPacket(sensors, pos, &outData);
	}

	// Send past 5 minutes of packets every 3 seconds
	if(++monCalls == 3) {
		Util::dumpHex(outData, 300*64);
		Radio::sendData(outData, 300*64);
		monCalls = 0;
	}
	delete pos;
}

int main()
{
	spdlog::info("AllTrax SR Telemetry TX " VERSION);
	spdlog::info("HIDAPI " HID_API_VERSION_STR);
	spdlog::set_level(spdlog::level::debug);

	Radio::init("/dev/ttyUSB0");
	GPS::init();

	Alltrax::setMonitorCallback(&monitor_callback);
#ifndef USE_FAKE_CONTROLLER
	if(!Alltrax::initMotorController())
#else
	if(!Alltrax::initMotorController(true))
#endif
		return -1;
	Alltrax::startMonitor(1);
	while(Alltrax::monThreadRunning);

	Alltrax::cleanup();
	spdlog::info("Exiting...");
	return 0;
}
