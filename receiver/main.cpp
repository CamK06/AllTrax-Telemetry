#include "httplib.h"
#include "link.h"
#include "gps.h"
#include "version.h"
#include "packet.h"

#include <flog.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>

#define SERIAL_PORT "/dev/ttyUSB0"
#define HTTP_IP "0.0.0.0"
#define HTTP_PORT 8000

// Data
time_t lastRx = -1;
std::vector<time_t> times;
std::vector<sensor_data> sensors;
std::vector<gps_pos> positions;
std::vector<float> acceleration;
std::vector<int> packetsReceived;
std::vector<int> packetsLost;
std::string json;
std::string startJson;
int totalRx = 0;
int totalLost = 0;
int responseLen = 0;
int lastCharge = 100;
unsigned char* response = nullptr;

// Battery charge table
const float chargeTable[20][2] = {
    {12.7, 100},
    {12.6, 95},
    {12.5, 90},
    {12.46, 85},
    {12.42, 80},
    {12.37, 75},
    {12.32, 70},
    {12.26, 65},
    {12.20, 60},
    {12.13, 55},
    {12.06, 50},
    {11.98, 45},
    {11.90, 40},
    {11.83, 35},
    {11.75, 30},
    {11.67, 25},
    {11.58, 20},
    {11.45, 15},
    {11.31, 10},
    {10.50, 0}
};

// Thanks, stackoverflow lol
// https://stackoverflow.com/questions/9527960/how-do-i-construct-an-iso-8601-datetime-in-c
std::string getTime()
{
    timeval curTime;
    gettimeofday(&curTime, NULL);

    int milli = curTime.tv_usec / 1000;
    char buf[sizeof "2011-10-08T07:07:09.000Z"];
    char *p = buf + strftime(buf, sizeof buf, "%FT%T", gmtime(&curTime.tv_sec));
    sprintf(p, ".%dZ", milli);

    return buf;
}

int main()
{
	flog::info("Telemetry Receiver v{}", VERSION);
    TLink::init((char*)SERIAL_PORT, false);
    TLink::setRxCallback([](unsigned char* data, int len, int type) {

		//if(type == TLink::DataType::Data) {
		//	response = new unsigned char[len];
		//	memcpy(response, data, len);
		//	responseLen = len;
		//	return;
		//}

		// Decode the packet
		sensor_data sensorData;
		gps_pos gps;
		time_t timestamp;
		Telemetry::decodePacket(data, &sensorData, &gps, &timestamp);

		// Check if times has the timestamp already, i.e. we've already seen this packet
		if(std::find(times.begin(), times.end(), timestamp) != times.end())
			return;
		
        // Debug logging
	    flog::debug("Battery Voltage: {0:.1f}V", sensorData.battVolt);
	    flog::debug("Battery Current: {0:.1f}A", sensorData.battCur);
	    flog::debug("Throttle: {}%", sensorData.throttle);
	    flog::debug("Controller Temp: {0:.1f}C", sensorData.controlTemp);
	    flog::debug("Battery Temp: {0:.1f}C", sensorData.battTemp);
		flog::debug("GPS: Lat: {0:.6f}, Long: {1:.6f}", gps.latitude, gps.longitude);
		flog::debug("Power: {0:.1f}W", sensorData.battCur*sensorData.battVolt);
		flog::debug("Velocity: {0:.1f}m/s", gps.velocity);
		flog::debug("Key: {} Eco: {}", sensorData.pwrSwitch ? "ON" : "OFF", sensorData.userSwitch ? "ON" : "OFF");
		flog::debug("Onboard Time: {}", timestamp);

		// Save the data
		sensors.push_back(sensorData);
    	positions.push_back(gps);
    	times.push_back(timestamp);
    	lastRx = time(NULL);
		packetsReceived.push_back(++totalRx);
		
		// Calculate packet loss
		//packetsLost.push_back(-1);
		packetsLost.push_back(TLink::rxFails);

		// Calculate acceleration
		if(positions.size() > 1) {
			float accel = fabs((positions[positions.size()-1].velocity - positions[positions.size()-2].velocity)/(times[times.size()-1]-times[times.size()-2])); // dV/dT
			acceleration.push_back(accel);
			flog::debug("Acceleration: {0:.1f}m/s^2", accel);
		}
		else
			acceleration.push_back(0);

		// Update the json with new data
    	nlohmann::json j;
    	for(int i = 0; i < sensors.size(); i++) { // Add data to the json file

        	// Calculate approximate charge percentage, excluding times when throttle
			// is applied because otherwise the voltage drop makes it inaccurate
			if(sensors[i].throttle <= 0) { 
        		for(int k = 0; k < 11; k++)
            	    if(sensors[i].battVolt <= chargeTable[k][0])
            	        lastCharge = chargeTable[k][1];
					else // TODO: Handle 24V here, right now 24V will always show 100%
						lastCharge = 100;
			}

			j["packets"][i]["chargePcnt"] = lastCharge;
        	j["packets"][i]["time"] = times[i];
        	j["packets"][i]["timeMs"] = times[i]*1000;
        	j["packets"][i]["throttle"] = sensors[i].throttle;
        	j["packets"][i]["battVolt"] = sensors[i].battVolt;
        	j["packets"][i]["battCur"] = sensors[i].battCur;
        	j["packets"][i]["battTemp"] = sensors[i].battTemp;
        	j["packets"][i]["power"] = sensors[i].battCur*sensors[i].battVolt;
        	j["packets"][i]["lat"] = positions[i].latitude;
        	j["packets"][i]["long"] = positions[i].longitude;
			j["packets"][i]["velocity"] = positions[i].velocity*3.6; // km/h
			j["packets"][i]["rxCount"] = packetsReceived[i];
			j["packets"][i]["lostCount"] = packetsLost[i];
			j["packets"][i]["index"] = packetsReceived[i] + packetsLost[i];
			j["packets"][i]["acceleration"] = acceleration[i]; // m/s^2 TODO: Convert to km/h
			j["packets"][i]["userSw"] = sensors[i].userSwitch ? 1 : 0;
			j["packets"][i]["keySw"] = sensors[i].pwrSwitch ? 1 : 0;
    	}
		json = j.dump(4);
	    printf("\n");
    });

	// Start HTTP server to serve the telemetry data to grafana
	httplib::Server server;
	server.Get("/telemetry", [](const httplib::Request& req, httplib::Response& res) {
		res.set_content(json, "application/json");
	});
	server.Get("/toggle", [](const httplib::Request& req, httplib::Response& res) {
		TLink::sendData((unsigned char*)"TOGGLE", 6, TLink::DataType::Command, true);
		res.set_content("OK", "text/plain");

		nlohmann::json j;
		j[0]["start"] = getTime();
		startJson = j.dump(4);
	});
	server.Get("/start", [](const httplib::Request& req, httplib::Response& res) {
		res.set_content(startJson, "application/json");
	});
	server.listen(HTTP_IP, HTTP_PORT);

	// Cleanup
	//TLink::cleanup();
	return 0;
}
