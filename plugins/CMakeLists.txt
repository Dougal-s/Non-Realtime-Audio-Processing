cmake_minimum_required(VERSION 3.10)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -fvisibility=\"hidden\"")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_ASAN "-fno-omit-frame-pointer -fsanitize=address")
set(CMAKE_LINKER_FLAGS_ASAN "-fno-omit-frame-pointer -fsanitize=address")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Plugins)

add_subdirectory(common/fft common/fft)

add_subdirectory("Monoifier" "${CMAKE_HOST_SYSTEM_NAME}/Monoifier")
add_subdirectory("Freq Shifter" "${CMAKE_HOST_SYSTEM_NAME}/Freq Shifter")
add_subdirectory("Normalise" "${CMAKE_HOST_SYSTEM_NAME}/Normalise")
