cmake_minimum_required(VERSION 3.10)

project(Normalise VERSION 1.0.0)

configure_file(plugin.info.in Normalise/plugin.info)

add_library(normalise MODULE normalise.cpp)
set_target_properties(normalise PROPERTIES LIBRARY_OUTPUT_DIRECTORY Normalise)
set_target_properties(normalise PROPERTIES PREFIX "")
target_include_directories(normalise PRIVATE ../../include)
