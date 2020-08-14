cmake_minimum_required(VERSION 3.10)

project(Monoifier VERSION 1.0.0)

configure_file(plugin.info.in "Monoifier/plugin.info")

add_library(monoifier MODULE monoifier.cpp)

target_link_libraries(monoifier PRIVATE FFT)

set_target_properties(monoifier PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Monoifier")
set_target_properties(monoifier PROPERTIES PREFIX "")

target_include_directories(monoifier PRIVATE ../../include)
