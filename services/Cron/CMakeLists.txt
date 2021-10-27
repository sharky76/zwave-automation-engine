cmake_minimum_required(VERSION 3.0.0)

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})

FILE(GLOB Files *.c)
add_library(${ProjectId} SHARED
${Files}
)
set_target_properties(${ProjectId} PROPERTIES LINKER_LANGUAGE C)

add_custom_command(TARGET ${ProjectId} POST_BUILD
COMMAND scp ${CMAKE_CURRENT_BINARY_DIR}/lib${ProjectId}.so osmc@osmc:/home/osmc/Projects/zwave-automation-engine/services/
)
