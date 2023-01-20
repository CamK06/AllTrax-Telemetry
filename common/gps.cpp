#include <spdlog/spdlog.h>
#include "gps.h"

namespace GPS
{

gpsmm* gpsRec;

int init()
{
    // Connect to gpsd
    gpsRec = new gpsmm("localhost", DEFAULT_GPSD_PORT);
    if(gpsRec->stream(WATCH_ENABLE | WATCH_JSON) == nullptr) {
        spdlog::error("GPSD: No GPSD running.");
        return false;;
    }
    return true;
}

gps_pos* getPosition()
{
    // Attempt to read GPS data
    struct gps_data_t* gpsData;
    if((gpsData = gpsRec->read()) == nullptr) {
        spdlog::error("GPSD: No data from GPSD.");
        return nullptr;
    }

    // Return the position
    gps_pos* pos = new gps_pos;
    pos->latitude = gpsData->fix.latitude;
    pos->longitude = gpsData->fix.longitude;
    return pos;
}

}