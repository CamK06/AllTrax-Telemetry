cmake_minimum_required(VERSION 3.1)
project(alltrax)

find_package(QT NAMES Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB SOURCES "*.cpp" "**/*.cpp" "../common/src/*.cpp") 
add_executable(telemetry-gui ${SOURCES})
target_link_libraries(telemetry-gui PRIVATE spdlog::spdlog Qt6::Widgets)
