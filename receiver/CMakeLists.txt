cmake_minimum_required(VERSION 3.1)
project(alltrax)

find_package(nlohmann_json REQUIRED)

file(GLOB SOURCES "*.cpp" "**/*.cpp" "../common/*.cpp") 
add_executable(telem-receiver ${SOURCES})
include_directories(cpp-httplib)
target_link_libraries(telem-receiver PRIVATE spdlog::spdlog nlohmann_json::nlohmann_json ${GPS_LIBRARIES})