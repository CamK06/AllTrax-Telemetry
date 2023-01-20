cmake_minimum_required(VERSION 3.1)
project(alltrax)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)
find_package(GPS REQUIRED)
include_directories(${SPDLOG_INCLUDE_DIR} ${HIDAPI_INCLUDE_DIRS} ${GPS_INCLUDE_DIR} common)

add_subdirectory(onboard)
add_subdirectory(receiver)
