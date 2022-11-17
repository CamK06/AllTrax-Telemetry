#pragma once

#include <cstring>
#include <cstdint>

namespace AllTraxVars
{

typedef struct memUint32
{
    uint32_t val;
    uint32_t addr;
} memUint32;

typedef struct memInt32
{
    int val;
    uint32_t addr;
} memInt32;

typedef struct memInt16
{
    int16_t val;
    uint32_t addr;
} memInt16;

typedef struct memString
{
    char* text;
    size_t len;
    uint32_t addr;
} memString;

memString modelName = { new char[15], 15, 134218752u };
memString buildDate = { new char[15], 15, 134218768u };
memUint32 serialNum = { 0, 134218784u };
memInt32 bootRev = { 0, 134242288u };
memInt32 originalBootRev = { 0, 134218788u };
memInt32 originalProgramRev = { 0, 134218792u };
memInt32 programType = { 0, 134218796u };
memInt16 startupType = { 0 , 134222848u };
memInt32 hardwareRev = { 0, 134218800u };

}