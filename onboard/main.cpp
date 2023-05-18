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
#include <string>
#include <fstream>
#include <iomanip>

#define SERIAL_PORT "/dev/ttyUSB0"

int monCalls = 0;
unsigned char* outData = nullptr;
bool transmitting = false;
#ifdef WRITE_LOCAL
std::string localDataFile;
std::ofstream localData;
bool openedData = false; // this is super dumb
#endif

void radio_callback(unsigned char* data, int len, int type)
{
	if(type != TLink::DataType::Command)
		return;

	// Decode the command
	if(memcmp(data, "TOGGLE", len) == 0)
		transmitting = !transmitting;
}

void monitor_callback(sensor_data* sensors)
{
	if(!transmitting)
		return;

	// Get GPS position or fake it
	gps_pos* pos = nullptr;
#ifndef USE_FAKE_GPS
	pos = GPS::getPosition();
	if(pos == nullptr) {
		pos = new gps_pos;
		pos->latitude = 0;
		pos->longitude = 0;
		pos->velocity = 0;
	}
#else 
 	pos->latitude = sin(random())*85;
 	pos->longitude = cos(random())*80;
 	pos->velocity = fabs((sin(random()*10)*cos(random()*20))*35);
#endif
#ifdef WRITE_LOCAL
	// Write the data to a file
	localData.open(localDataFile, std::ofstream::app);
	if(!openedData) {
		localData << "voltage,current,speed,throttle,powerSw,ecoSw,controlTemp,battTemp,lat,long,time" << std::endl;
		openedData = true;
	}
	localData << sensors->battVolt << ',' << sensors->battCur << ',' << pos->velocity << ',' << sensors->throttle << ',' << sensors->pwrSwitch << ',' << sensors->userSwitch << ',' << sensors->controlTemp << ',' << sensors->battTemp << ',' << pos->latitude << ',' << pos->longitude << ',' << time(nullptr) << std::endl;	
	localData.close();
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
		//Util::dumpHex(outData, PKT_BURST*PKT_LEN);
		TLink::sendData(outData, PKT_BURST*PKT_LEN, TLink::DataType::Data, true);
		monCalls = 0;
	}
}

int main()
{
	flog::info("AllTrax SR Telemetry TX " VERSION);
	flog::info("HIDAPI " HID_API_VERSION_STR);

	TLink::init((char*)SERIAL_PORT, true);
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

#ifdef WRITE_LOCAL
	// Create the file to write local data to
	std::ostringstream str;
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	str << "telemetry-" << std::put_time(&tm, "%d-%m-%y %H-%M-%S") << ".csv";
	localDataFile = str.str(); 
#endif

	// Begin monitor mode
	Alltrax::startMonitor(PKT_RATE);
	while(Alltrax::monThreadRunning);

	Alltrax::cleanup();
	//TLink::cleanup();
#ifndef USE_FAKE_GPS
	GPS::close();
#endif
	flog::info("Exiting...");
	return 0;
}
