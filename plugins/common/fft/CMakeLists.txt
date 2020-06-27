cmake_minimum_required(VERSION 3.10)
add_compile_options(-fPIC)
add_library(FFT STATIC fft.cpp fft.hpp)
target_include_directories(FFT PUBLIC ${CMAKE_CURRENT_LIST_DIR})
