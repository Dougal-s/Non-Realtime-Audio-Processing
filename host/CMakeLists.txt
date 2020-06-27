cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -fvisibility=\"hidden\"")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_ASAN "-fno-omit-frame-pointer -fsanitize=address")
set(CMAKE_LINKER_FLAGS_ASAN "-fno-omit-frame-pointer -fsanitize=address")

project(Host VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_executable(host
	src/audio.cpp
	src/Dynamic_Library.cpp
	src/main.cpp
	src/plugin.cpp
)

target_include_directories(host PUBLIC include)
target_include_directories(host PUBLIC ../include)

if (UNIX)
	target_link_libraries(host PUBLIC dl pthread)
endif ()
