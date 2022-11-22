#include "alltraxvars.h"

namespace Vars
{
VarString model("Model", 15, 134218752u);
VarString buildDate("BuildDate", 15, 134218768u);
VarU32 serialNum("SerialNum", 134218784u, 0, UINT32_MAX, 1.0, 0, "");
VarInt32 bootRev("BootRev", 134242288u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");
VarInt32 originalBootRev("OriginalBootRev", 134218788u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");
VarInt32 originalProgRev("OriginalProgRev", 134218792u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");
VarInt32 programType("ProgramType", 134218796u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");
VarU16 startUpSwitch("StartupSwitch", 134222848u, 0, INT16_MAX, 1.0, 0, "");
VarInt32 hardwareRev("HardwareRev", 134218800u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");

Var infoVars[] = {  Vars::model, Vars::buildDate, Vars::serialNum,
                    Vars::bootRev, Vars::originalBootRev, Vars::originalProgRev,
                    Vars::programType, Vars::startUpSwitch, Vars::hardwareRev };
}