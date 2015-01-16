cmake_minimum_required(VERSION 2.8.4)
project(melon)

include_directories("/usr/include")
link_directories("/usr/lib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wall -pg")
set(SOURCE_FILES main.c fifo.c threads.c thread_pool.c)

add_executable(melon ${SOURCE_FILES})
target_link_libraries(melon)
