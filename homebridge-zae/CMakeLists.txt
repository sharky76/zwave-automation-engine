cmake_minimum_required(VERSION 3.0.0)

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})

FILE(GLOB Files *.js)

add_custom_target(${ProjectId} ALL 
COMMAND scp ${Files} osmc@osmc:/home/osmc/Projects/zwave-automation-engine/homebridge-zae/
)