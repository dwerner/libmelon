cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
project(melon)

find_package(Check REQUIRED)

include_directories(/usr/include include/)
link_directories("/usr/lib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wall -pg")
set(SOURCE_FILES
src/main.c
src/fifo.c
src/threads.c
src/thread_pool.c
src/actor.c
src/promise.c
src/actor_system.c)

add_executable(melon ${SOURCE_FILES})
target_link_libraries(melon)
