#include "httplib.h"
#include "link.h"
#include "gps.h"
#include "version.h"
#include "packet.h"

#include <flog.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <fstream>

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
int totalRx = 0;
int totalLost = 0;

// Battery charge table
const float chargeTable[11][2] = {
    {12.7, 100},
    {12.5, 90},
    {12.42, 80},
    {12.32, 70},
	{12.20, 60},
	{12.06, 50},
	{11.90, 40},
	{11.75, 30},
	{11.58, 20},
	{11.31, 10},
	{10.50, 0}
};

int main()
{
	flog::info("Telemetry Receiver v{}", VERSION);
    TLink::init(SERIAL_PORT, true);
    TLink::setRxCallback([](unsigned char* data, int len) {

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

		// Save the data
		sensors.push_back(sensorData);
    	positions.push_back(gps);
    	times.push_back(timestamp);
    	lastRx = time(NULL);
		packetsReceived.push_back(++totalRx);
		
		// Calculate packet loss
		packetsLost.push_back(TLink::framesLost);

		// Calculate acceleration
		if(positions.size() > 1) {
			float accel = fabs(positions[positions.size()-1].velocity - positions[positions.size()-2].velocity);
			acceleration.push_back(accel);
			flog::debug("Acceleration: {0:.1f}m/s^2", accel);
		}
		else
			acceleration.push_back(0);

		// Update the json with new data
    	nlohmann::json j;
    	for(int i = 0; i < sensors.size(); i++) { // Add data to the json file

        	// Calculate approximate charge percentage
        	if(sensors[i].battVolt < 15) // We are likely running 12V
            	for(int k = 0; k < 11; k++)
                	if(sensors[i].battVolt <= chargeTable[k][0])
            	        j["packets"][i]["chargePcnt"] = chargeTable[k][1];
			else
				j["packets"][i]["chargePcnt"] = 0;

        	j["packets"][i]["time"] = times[i];
        	j["packets"][i]["timeMs"] = times[i]*1000;
        	j["packets"][i]["throttle"] = sensors[i].throttle;
        	j["packets"][i]["battVolt"] = sensors[i].battVolt;
        	j["packets"][i]["battCur"] = sensors[i].battCur;
        	j["packets"][i]["battTemp"] = sensors[i].battTemp;
        	j["packets"][i]["power"] = sensors[i].battCur*sensors[i].battVolt;
        	j["packets"][i]["lat"] = positions[i].latitude;
        	j["packets"][i]["long"] = positions[i].longitude;
			j["packets"][i]["velocity"] = positions[i].velocity*3.6;
			j["packets"][i]["rxCount"] = packetsReceived[i];
			j["packets"][i]["lostCount"] = packetsLost[i];
			j["packets"][i]["index"] = packetsReceived[i] + packetsLost[i];
			j["packets"][i]["acceleration"] = acceleration[i];
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
	server.listen(HTTP_IP, HTTP_PORT);
}