#include "alltrax.h"
#include "version.h"

#include <spdlog/spdlog.h>
#include <fstream>

void receive_callback(unsigned char* data, size_t len)
{
	// Dump the hex data to the terminal
	// This is a ghetto way of doing this but idgaf
	spdlog::debug("Data received from USB:");
	for(int i = 0; i < len; i++) {
		printf("%x ", data[i]);
		if(i == 15 || i == 31 || i == 47)
			printf("\n");
	}
	printf("\n");

	// Dump the data to a file
	std::ofstream file;
	file.open("out.bin");
	file.write((const char*)data, len);
	file.close();
	spdlog::debug("Data received from USB written to out.bin");
}

int main()
{
	spdlog::info("AllTrax SR Telemetry TX " VERSION);
	spdlog::info("HIDAPI " HID_API_VERSION_STR);
	spdlog::set_level(spdlog::level::debug);

	// Find and initialize the motor controller
	Alltrax::setReceiveCallback(receive_callback);
	if(!Alltrax::initMotorController()) {
		spdlog::error("No motor controller found!");
		spdlog::error("Quitting...");
		return -1;
	}

	// Begin reading data from the controller
	Alltrax::beginRead();

	while(true);

	// Clean up and exit
	Alltrax::cleanup();
	return 0;
}
