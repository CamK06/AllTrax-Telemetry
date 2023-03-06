cmake_minimum_required(VERSION 3.1)
project(alltrax)

file(GLOB SOURCES "*.cpp" "**/*.cpp" "../common/*.cpp" "../thirdparty/flog/lib/*.cpp")
add_executable(alltrax ${SOURCES})
target_link_libraries(alltrax PRIVATE hidapi-hidraw ${GPS_LIBRARIES} ZLIB::ZLIB)
