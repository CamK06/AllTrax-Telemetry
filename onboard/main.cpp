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
	gps_pos* pos = GPS::getPosition();
	
	unsigned char* outData = new unsigned char[32];
	Telemetry::formatPacket(sensors, pos, &outData);
	Util::dumpHex(outData, 32);
	Radio::sendSensors(sensors, pos);
}

int main()
{
	spdlog::info("AllTrax SR Telemetry TX " VERSION);
	spdlog::info("HIDAPI " HID_API_VERSION_STR);
	spdlog::set_level(spdlog::level::debug);

	Radio::init();
	GPS::init();

	Alltrax::setMonitorCallback(&monitor_callback);
	if(!Alltrax::initMotorController())
		return -1;
	Alltrax::startMonitor();
	while(Alltrax::monThreadRunning);

	Alltrax::cleanup();
	spdlog::info("Exiting...");
	return 0;
}
