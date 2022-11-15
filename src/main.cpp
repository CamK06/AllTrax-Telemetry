#include <spdlog/spdlog.h>
#include "alltrax.h"
#include "version.h"

int main()
{
	spdlog::info("AllTrax SR Telemetry " VERSION);
	spdlog::set_level(spdlog::level::debug);

	// Find and initialize the motor controller
	Alltrax::initializeLibUSB();
	if(!Alltrax::findMotorController()) {
		spdlog::error("No motor controller found!");
		spdlog::error("Quitting...");
		return -1;
	}

	return 0;
}
