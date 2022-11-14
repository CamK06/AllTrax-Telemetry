#include <iostream>
#include <libusb-1.0/libusb.h>

int main()
{
	int result = libusb_init(NULL);
	std::cout << "Initialized libUSB" << std::endl;
}
