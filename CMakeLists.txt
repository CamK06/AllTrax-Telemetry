cmake_minimum_required(VERSION 3.1)
project(alltrax)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)
find_package(hidapi REQUIRED)
#pkg_check_modules(libusb REQUIRED libusb-1.0)

file(GLOB SOURCES "src/*.cpp" "src/**/*.cpp")

add_executable(alltrax ${SOURCES})
include_directories(${SPDLOG_INCLUDE_DIR} ${HIDAPI_INCLUDE_DIRS}) # ${LIBUSB_INCLUDE_DIRS})
target_link_libraries(alltrax PRIVATE spdlog::spdlog hidapi-libusb) # usb-1.0
