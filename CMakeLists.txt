cmake_minimum_required(VERSION 3.0)
project(c-processor-emulator)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES Clang OR ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  # using regular Clang or AppleClang or GCC
  set(CMAKE_C_FLAGS "-pedantic -Wall -Wextra -g -O0")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  # using Visual Studio C++
  set(CMAKE_CXX_FLAGS "/permissive- /W4 /EHsc")
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif()

add_executable(cpu "main.c" "cpu.c")

target_compile_definitions(compiler PUBLIC -D_POSIX_C_SOURCE=200809L )
