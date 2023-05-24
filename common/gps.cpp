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
    if(gpsRec->stream(WATCH_ENABLE) == nullptr) {
        flog::error("GPSD: No GPSD running.");
        return false;
    }

    return true;
}

gps_pos* getPosition()
{
    // NOTE:
    // It might be worth creating a for loop which reads from gps repeatedly
    // until the read data's GPS timestamp is within x seconds of the current time (5?)
    // Only then do we know the data is accurate
    // It also might be worth using the raw C API instead of gpsmm

    // Attempt to read GPS data
    if(!gpsRec->waiting(500000)) {
        flog::error("GPSD: No data from GPSD. Timeout.");
        return pos;
    }
    struct gps_data_t* gpsData;
    if((gpsRec->read()) == nullptr) {
        flog::error("GPSD: No data from GPSD.");
        return pos;
    }
    if(((gpsData = gpsRec->read()) == nullptr) || (gpsData->fix.mode < MODE_2D)) {
        flog::error("GPSD: No fix.");
        return pos;
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
    pos->time = gpsData->fix.time;
    return pos;
}

void close()
{
    delete gpsRec;
    delete pos;
}

}
