cmake_minimum_required(VERSION 3.10)

project(FreqShifter VERSION 1.0.0)

configure_file(plugin.info.in "Freq Shifter/plugin.info")

add_library(freq_shifter MODULE freq_shifter.cpp)

target_link_libraries(freq_shifter PRIVATE FFT)

set_target_properties(freq_shifter PROPERTIES LIBRARY_OUTPUT_DIRECTORY "Freq Shifter")
set_target_properties(freq_shifter PROPERTIES PREFIX "")

target_include_directories(freq_shifter PRIVATE ../../include)
