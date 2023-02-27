cmake_minimum_required(VERSION 3.1)
project(alltrax)

find_package(nlohmann_json REQUIRED)

file(GLOB SOURCES "*.cpp" "**/*.cpp" "../common/*.cpp" "../thirdparty/flog/lib/*.cpp") 
add_executable(telem-receiver ${SOURCES})
include_directories(cpp-httplib)
target_link_libraries(telem-receiver PRIVATE nlohmann_json::nlohmann_json ${GPS_LIBRARIES} ZLIB::ZLIB)