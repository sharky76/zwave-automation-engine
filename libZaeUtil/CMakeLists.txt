cmake_minimum_required(VERSION 3.0.0)

FILE(GLOB LibZaeUtil *.c)
add_library(ZaeUtil SHARED
${LibZaeUtil}
)
set_target_properties(ZaeUtil PROPERTIES LINKER_LANGUAGE C)