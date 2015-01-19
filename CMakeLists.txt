cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
project(melon)
enable_testing()
add_test( melon-tests melon-tests )
find_package(Check REQUIRED)
include_directories(/usr/include ${CMAKE_CURRENT_SOURCE_DIR}/include/)

link_directories("/usr/lib")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wall -pg")
set(SOURCE_FILES
src/fifo.c
src/threads.c
src/thread_pool.c
src/actor.c
src/promise.c
src/actor_system.c
src/message.c)

add_library(melon ${SOURCE_FILES})
target_include_directories (melon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
add_executable(melon-tests tests/main.c)
target_link_libraries(melon-tests LINK_PUBLIC melon)
