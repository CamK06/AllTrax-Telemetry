#include <spdlog/spdlog.h>
#include "gps.h"

namespace GPS
{

gpsmm* gpsRec;

int init()
{
#ifndef USE_FAKE_CONTROLLER
    // Connect to gpsd
    gpsRec = new gpsmm("localhost", DEFAULT_GPSD_PORT);
    if(gpsRec->stream(WATCH_ENABLE | WATCH_JSON) == nullptr) {
        spdlog::error("GPSD: No GPSD running.");
        return false;;
    }
#endif
    return true;
}

gps_pos* getPosition()
{
    // Attempt to read GPS data
    if(!gpsRec->waiting(5000)) {
        spdlog::error("GPSD: No data from GPSD. Timeout.");
        return nullptr;
    }
    struct gps_data_t* gpsData;
    if((gpsData = gpsRec->read()) == nullptr) {
        spdlog::error("GPSD: No data from GPSD.");
        return nullptr;
    }

    // Return the position
    gps_pos* pos = new gps_pos;
    pos->latitude = gpsData->fix.latitude;
    pos->longitude = gpsData->fix.longitude;
    pos->velocity = gpsData->fix.speed;
    return pos;
}

}
