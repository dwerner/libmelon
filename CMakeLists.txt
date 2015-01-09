cmake_minimum_required(VERSION 2.8.4)
project(highchair)

include_directories("/usr/include")
link_directories("/usr/lib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wall -pg")
set(SOURCE_FILES main.c fifo.c)

add_executable(highchair ${SOURCE_FILES})
target_link_libraries(highchair)