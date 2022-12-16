#pragma once

#include "alltraxvars/var.h"
#include "alltraxvars/varbool.h"
#include "alltraxvars/varint8.h"
#include "alltraxvars/varint16.h"
#include "alltraxvars/varint32.h"
#include "alltraxvars/varstring.h"
#include "alltraxvars/varu8.h"
#include "alltraxvars/varu16.h"
#include "alltraxvars/varu32.h"

// ALL Controller model vars:
namespace Vars
{

extern VarString model;
extern VarString buildDate;
extern VarString throttleTypeName;
extern VarInt32 bootRev;
extern VarInt32 originalBootRev;
extern VarInt32 originalProgRev;
extern VarInt32 programType;
extern VarInt32 hardwareRev;
extern VarU32 serialNum;
extern VarU16 startUpSwitch;
extern VarU8 throttleType;
extern Var* infoVars[];

extern VarInt16 battVoltage;
extern VarInt16 throttleLocal;
extern VarInt16 throttlePos;
extern VarInt16 throttlePoint;
extern VarInt32 outputAmps;
extern VarInt16 battTemp;
extern VarInt16 mcuTemp;
extern VarBool keySwitch;
extern VarBool userSwitch;
extern Var* telemetryVars[];
}