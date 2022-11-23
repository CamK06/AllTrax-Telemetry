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
	file.open("rec.bin");
	file.write((const char*)data, len);
	file.close();
	spdlog::debug("Data received from USB written to out.bin");
}

int main()
{
	spdlog::info("AllTrax SR Telemetry TX " VERSION);
	spdlog::info("HIDAPI " HID_API_VERSION_STR);
	spdlog::set_level(spdlog::level::debug);

	Alltrax::setReceiveCallback(&receive_callback);
	Alltrax::initMotorController();
	return 0;
}
