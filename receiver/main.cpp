#include "httplib.h"
#include "radio.h"
#include "gps.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <fstream>

#define SERIAL_PORT "/dev/ttyUSB0"
#define HTTP_IP "0.0.0.0"
#define HTTP_PORT 8000
#define PACKET_INTERVAL 3 // Seconds, used for calculating packet loss

// Data
time_t lastRx = -1;
std::vector<time_t> times;
std::vector<sensor_data> sensors;
std::vector<gps_pos> positions;
std::string json;
std::vector<int> packetsReceived;
std::vector<int> packetsLost;
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
    Radio::init(SERIAL_PORT, true);
    Radio::setRxCallback([](sensor_data sensorData, gps_pos gps) {
        // Debug logging
	    spdlog::debug("Battery Voltage: {0:.1f}V", sensorData.battVolt);
	    spdlog::debug("Battery Current: {0:.1f}A", sensorData.battCur);
	    spdlog::debug("Throttle: {}%", sensorData.throttle);
	    spdlog::debug("Controller Temp: {0:.1f}C", sensorData.controlTemp);
	    spdlog::debug("Battery Temp: {0:.1f}C", sensorData.battTemp);
		spdlog::debug("GPS: Lat: {0:.6f}, Long: {1:.6f}", gps.latitude, gps.longitude);
		spdlog::debug("Power: {0:.1f}W", sensorData.battCur*sensorData.battVolt);

		// Save the data
		sensors.push_back(sensorData);
    	positions.push_back(gps);
    	times.push_back(time(NULL));
    	lastRx = time(NULL);
		packetsReceived.push_back(++totalRx);
		
		// Calculate packet loss
		if(times.size() > 1) {
			int total = (times[times.size()-1] - times[0]) / PACKET_INTERVAL;
			totalLost = total - totalRx + 1;
			packetsLost.push_back(totalLost);
		}
		else
			packetsLost.push_back(0);

		// Update the json with new data
    	nlohmann::json j;
    	for(int i = 0; i < sensors.size(); i++) { // Add data to the json file

        	// Calculate approximate charge percentage
        	if(sensors[i].battVolt < 15) // We are likely running 12V
            	for(int k = 0; k < 11; k++)
                	if(sensors[i].battVolt <= chargeTable[k][0])
            	        j["packets"][i]["chargePcnt"] = chargeTable[k][1];

        	j["packets"][i]["time"] = times[i];
        	j["packets"][i]["timeMs"] = times[i]*1000;
        	j["packets"][i]["throttle"] = sensors[i].throttle;
        	j["packets"][i]["battVolt"] = sensors[i].battVolt;
        	j["packets"][i]["battCur"] = sensors[i].battCur;
        	j["packets"][i]["battTemp"] = sensors[i].battTemp;
        	j["packets"][i]["power"] = sensors[i].battCur*sensors[i].battVolt;
        	j["packets"][i]["lat"] = positions[i].latitude;
        	j["packets"][i]["long"] = positions[i].longitude;
			j["packets"][i]["rxCount"] = packetsReceived[i];
			j["packets"][i]["lostCount"] = packetsLost[i];
			j["packets"][i]["index"] = packetsReceived[i] + packetsLost[i];
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