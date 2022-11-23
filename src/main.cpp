#include "alltrax.h"
#include "version.h"

#include <spdlog/spdlog.h>
#include <fstream>
#include <thread>
#include <chrono>

void receive_callback(unsigned char* data, size_t len)
{
	// Dump the hex data to the terminal
	// This is a ghetto way of doing this but idgaf
	spdlog::debug("Data received from USB:");
	for(int i = 0; i < len; i++) {
		printf("%x ", data[i]);
		if(i == 15 || i == 31 || i == 47)
			printf("\n");
	}
	printf("\n");

	// Dump the data to a file
	std::ofstream file;
	file.open("rec.bin");
	file.write((const char*)data, len);
	file.close();
	spdlog::debug("Data received from USB written to out.bin");
}

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

int main()
{
	spdlog::info("AllTrax SR Telemetry TX " VERSION);
	spdlog::info("HIDAPI " HID_API_VERSION_STR);
	spdlog::set_level(spdlog::level::debug);

	Alltrax::setReceiveCallback(&receive_callback);
	//Alltrax::beginRead();
	bool running = Alltrax::initMotorController();

	// Monitor mode
	// NOTE: Perhaps it's a better idea to have this in alltrax.cpp and use a callback similar to receive_callback? That way this file can be strictly radio/general data processing
	sensor_data* sensors = (sensor_data*)malloc(sizeof(sensor_data));
	sensors->battVolt = 0;
	sensors->battCur = 0;
	sensors->throttle = 0;
	sensors->battTemp = 0;
	sensors->controlTemp = 0;
	while(running) {
		Alltrax::readVars(Vars::telemetryVars, 4);
		sensors->battVolt = Vars::battVoltage.convertToReal();
		sensors->battCur = Vars::outputAmps.convertToReal() * (double)Vars::throttleLocal.getVal() / 4095.0;
		sensors->throttle = 100.0 * Vars::throttlePos.convertToReal() / 4095.0;
		sensors->controlTemp = Vars::mcuTemp.convertToReal();
		sensors->battTemp = Vars::battTemp.convertToReal();
		// if(!Alltrax::readSensors(&sensors)) {
		//	spdlog::error("Failed to read sensors!");
		//	spdlog::error("Exiting...");
		//	return -1;
		// }
		spdlog::debug("Sensors:");
		printSensors(sensors);
		// TRANSMIT DATA HERE
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	return 0;
}
