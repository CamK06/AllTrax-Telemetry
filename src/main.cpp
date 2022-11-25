#include "alltrax/alltrax.h"
#include "version.h"

#include <spdlog/spdlog.h>
#include <fstream>
#include <signal.h>

void printSensors(sensor_data* sensors)
{
	// Battery
	spdlog::debug("Battery Voltage: {}V", sensors->battVolt);
	spdlog::debug("Battery Current: {}A", sensors->battCur);

	// Motor
	
	// Misc
	spdlog::debug("Throttle: {}%", sensors->throttle);
	spdlog::debug("Controller Temp: {}C", sensors->controlTemp);
	spdlog::debug("Battery Temp: {}C", sensors->battTemp);
	printf("\n");	
}

void monitor_callback(sensor_data* sensors)
{
	printSensors(sensors);
	// TRANSMIT HERE
}

int main()
{
	spdlog::info("AllTrax SR Telemetry TX " VERSION);
	spdlog::info("HIDAPI " HID_API_VERSION_STR);
	spdlog::set_level(spdlog::level::debug);

	Alltrax::setMonitorCallback(&monitor_callback);
	if(!Alltrax::initMotorController())
		return -1;
	Alltrax::startMonitor();
	while(Alltrax::monThreadRunning);

	Alltrax::cleanup();
	return 0;
}
