cmake_minimum_required(VERSION 3.1)
project(alltrax)

find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)
pkg_check_modules(libusb REQUIRED libusb-1.0)

file(GLOB SOURCES "src/*.cpp")

add_executable(alltrax ${SOURCES})
include_directories(${LIBUSB_INCLUDE_DIR} ${SPDLOG_INCLUDE_DIR})
target_link_libraries(alltrax PRIVATE usb-1.0 spdlog::spdlog)
