#include "version.h"
#include "alltrax/alltrax.h"
#include "link.h"
#include "packet.h"
#include "util.h"
#include "gps.h"

#include <spdlog/spdlog.h>
#include <signal.h>
#include <vector>

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
	spdlog::info("AllTrax SR Telemetry TX " VERSION);
	spdlog::info("HIDAPI " HID_API_VERSION_STR);
	spdlog::set_level(spdlog::level::debug);

	TLink::init("/dev/ttyUSB1");
	GPS::init();

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
	spdlog::info("Exiting...");
	return 0;
}
