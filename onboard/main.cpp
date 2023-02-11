#define USE_FAKE_CONTROLLER true

#include "alltrax/alltrax.h"
#include "packet.h"
#include "radio.h"
#include "version.h"
#include "util.h"
#include "gps.h"

#include <spdlog/spdlog.h>
#include <signal.h>

void printSensors(sensor_data* sensors)
{
	// Battery
	spdlog::debug("Battery Voltage: {0:.1f}V", sensors->battVolt);
	spdlog::debug("Battery Current: {0:.1f}A", sensors->battCur);
	
	// Misc
	spdlog::debug("Throttle: {}%", sensors->throttle);
	spdlog::debug("Controller Temp: {0:.1f}C", sensors->controlTemp);
	spdlog::debug("Battery Temp: {0:.1f}C", sensors->battTemp);
	printf("\n");	
}

void monitor_callback(sensor_data* sensors)
{
	printSensors(sensors);
	gps_pos* pos = (gps_pos*)malloc(sizeof(gps_pos));
#ifndef USE_FAKE_CONTROLLER
	pos = GPS::getPosition();
#else
	pos->latitude = sin(random())*85;
	pos->longitude = cos(random())*80;
	pos->velocity = fabs((sin(random()*10)*cos(random()*20))*35);
#endif
	
	unsigned char* outData = new unsigned char[64];
	Telemetry::formatPacket(sensors, pos, &outData);
	Util::dumpHex(outData, 64);
	Radio::sendSensors(sensors, pos);
	delete pos;
	delete outData;
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
	Alltrax::startMonitor(3);
	while(Alltrax::monThreadRunning);

	Alltrax::cleanup();
	spdlog::info("Exiting...");
	return 0;
}
