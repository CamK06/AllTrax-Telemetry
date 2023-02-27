#pragma once

#include <libgpsmm.h>

struct gps_pos
{
    double latitude;
    double longitude;
    double velocity;
};

namespace GPS
{

int init();
void close();
gps_pos* getPosition();

}