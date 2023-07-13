cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER "/home/alex/x-tools/armv7-rpi2-linux-gnueabihf/bin/armv7-rpi2-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "/home/alex/x-tools/armv7-rpi2-linux-gnueabihf/bin/armv7-rpi2-linux-gnueabihf-gcc")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

message("-- ${CMAKE_BUILD_TYPE}")

add_compile_options(-c -x c "-D_GNU_SOURCE" -fPIC -std=c99)

SET(CMAKE_EXE_LINKER_FLAGS_Debug "-g -Wl,-Rlibs/ -Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/exports.txt")
SET(CMAKE_EXE_LINKER_FLAGS_Release "-Wl,-Rlibs/ -Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/exports.txt")

set(CMAKE_VERBOSE_MAKEFILE ON)

project(zwave-automation-engine VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories(
    libZaeUtil
    /home/alex/x-tools/armv7-rpi2-linux-gnueabihf/include 
    /home/alex/x-tools/armv7-rpi2-linux-gnueabihf/include/libzway
)

link_directories(
    /home/alex/x-tools/armv7-rpi2-linux-gnueabihf/lib
    /home/alex/x-tools/armv7-rpi2-linux-gnueabihf/lib/libzway
)

add_subdirectory(libZaeUtil)
add_subdirectory(services)
add_subdirectory(vdev)
add_subdirectory(scripts)
add_subdirectory(homebridge-zae)

FILE(GLOB AllSrc *.c BuiltinServices/*.c)
add_executable(zwave-automation-engine ${AllSrc})

add_dependencies(zwave-automation-engine 
Cron
DateTime
IFTTT
Mail
SMS
Timer
Weather
BlinkCamera
Button 
SecuritySystem
Sensor
SurveillanceStation
homebridge-zae
scripts
)

target_link_libraries(zwave-automation-engine 
ZaeUtil
zway
zcommons
xml2
z
m
crypto
archive
lzma
nettle
acl
attr
lzo2
bz2
curl
json-c
dl
python3.9
pthread
util
)

set_target_properties(zwave-automation-engine  PROPERTIES LINK_FLAGS "-Wl,-Rlibs/ -Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/exports.txt")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_custom_command(TARGET zwave-automation-engine POST_BUILD
COMMAND scp ${CMAKE_CURRENT_BINARY_DIR}/zwave-automation-engine osmc@osmc:/home/osmc/Projects/zwave-automation-engine/
)