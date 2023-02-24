cmake_minimum_required(VERSION 3.1)
project(alltrax)

file(GLOB SOURCES "*.cpp" "**/*.cpp" "../common/*.cpp")
add_executable(alltrax ${SOURCES})
target_link_libraries(alltrax PRIVATE spdlog::spdlog hidapi-libusb ${GPS_LIBRARIES} ZLIB::ZLIB)
