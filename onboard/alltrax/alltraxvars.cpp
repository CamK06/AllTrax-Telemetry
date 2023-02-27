#include "alltraxvars.h"

namespace Vars
{
VarString model("Model", 15, 134218752u);
VarString buildDate("BuildDate", 15, 134218768u);
VarString throttleTypeName("Throttle_Type_Name", 15, 134219904u);
VarInt32 bootRev("BootRev", 134242288u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");
VarInt32 originalBootRev("OriginalBootRev", 134218788u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");
VarInt32 originalProgRev("OriginalProgRev", 134218792u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");
VarInt32 programType("ProgramType", 134218796u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");
VarInt32 hardwareRev("HardwareRev", 134218800u, INT32_MIN, INT32_MAX, 1.0, 0, "N/A");
VarU32 serialNum("SerialNum", 134218784u, 0, UINT32_MAX, 1.0, 0, "");
VarU16 startUpSwitch("StartupSwitch", 134222848u, 0, INT16_MAX, 1.0, 0, "");
VarU16 lowBattLimit("F_LoBat_Vlim", 134219808u, 0, UINT16_MAX, 0.1, 0, "V");
VarU16 highBattLimit("F_HiBat_Vlim", 134219810u, 0, UINT16_MAX, 0.1, 0, "V");
VarU8 throttleType("Throttle_Type", 134219872u, 0, UINT8_MAX, 1.0, 0, "MU");

// Sensors
VarInt16 battVoltage("BPlus_Volts", 536887568u, INT16_MIN, INT16_MAX, 0.1, 0, "V");
VarInt16 throttleLocal("Throttle_Local", 536887574u, INT16_MIN, INT16_MAX, 1.0, 0, "N/A");
VarInt16 throttlePos("Throttle_Position", 536887576u, INT16_MIN, INT16_MAX, 1.0, 0, "N/A");
VarInt32 outputAmps("Output_Amps", 536887578u, INT32_MIN, INT32_MAX, 0.1, 0, "A");
VarInt16 battTemp("Avg_BPlusTemp", 536887526u, INT16_MIN, INT16_MAX, 0.1289, 527, "*C");
VarInt16 mcuTemp("Avg_MMinusTemp", 536887528u, INT16_MIN, INT16_MAX, 0.1289, 527, "*C");
VarBool keySwitch("KeySwitch_Input", 536887440u);
VarBool userSwitch("User1_Input", 536887448u);

Var* infoVars[] = {  
    &Vars::model, &Vars::buildDate, &Vars::serialNum,
    &Vars::bootRev, &Vars::originalBootRev, &Vars::originalProgRev,
};

Var* telemetryVars[] = { 
    &Vars::battVoltage, &Vars::throttleLocal, &Vars::throttlePos,
    &Vars::outputAmps, &Vars::battTemp, &Vars::mcuTemp,
    &Vars::keySwitch, &Vars::userSwitch
};
}