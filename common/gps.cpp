#include <flog.h>
#include "gps.h"

namespace GPS
{

gpsmm* gpsRec;
gps_pos* pos = nullptr; // Stores the last received position

int init()
{
    // Connect to gpsd
    gpsRec = new gpsmm("localhost", DEFAULT_GPSD_PORT);
    if(gpsRec->stream(WATCH_ENABLE | WATCH_JSON) == nullptr) {
        flog::error("GPSD: No GPSD running.");
        return false;;
    }
    return true;
}

gps_pos* getPosition()
{
    // Attempt to read GPS data
    if(!gpsRec->waiting(5000)) {
        flog::error("GPSD: No data from GPSD. Timeout.");
        return nullptr;
    }
    struct gps_data_t* gpsData;
    if((gpsData = gpsRec->read()) == nullptr) {
        flog::error("GPSD: No data from GPSD.");
        return nullptr;
    }

    // Verify the incoming data, if invalid, return last data
    if(gpsData->fix.latitude <= 0)
        return pos;

    // Return the position
    if(pos == nullptr)
        pos = new gps_pos;
    pos->latitude = gpsData->fix.latitude;
    pos->longitude = gpsData->fix.longitude;
    pos->velocity = gpsData->fix.speed;
    return pos;
}

void close()
{
    delete gpsRec;
    delete pos;
}

}
