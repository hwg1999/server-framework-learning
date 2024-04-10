cmake_minimum_required(VERSION 3.22)
project(sylar)

# set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(LIB_SRC sylar/log.cpp sylar/util.cpp sylar/config.cpp sylar/thread.cpp sylar/fiber.cpp sylar/scheduler.cpp
    sylar/iomanager.cpp)

include_directories(${CMAKE_SOURCE_DIR})
add_library(sylar SHARED ${LIB_SRC})

add_executable(log_test test/log_test.cpp)
target_link_libraries(log_test sylar yaml-cpp)

add_executable(config_test test/config_test.cpp)
target_link_libraries(config_test sylar yaml-cpp)

add_executable(thread_test test/thread_test.cpp)
target_link_libraries(thread_test sylar yaml-cpp pthread)

add_executable(util_test test/util_test.cpp)
target_link_libraries(util_test sylar yaml-cpp pthread)

add_executable(fiber_test test/fiber_test.cpp)
target_link_libraries(fiber_test sylar yaml-cpp pthread)

add_executable(scheduler_test test/scheduler_test.cpp)
target_link_libraries(scheduler_test sylar yaml-cpp pthread)

add_executable(iomanager_test test/iomanager_test.cpp)
target_link_libraries(iomanager_test sylar yaml-cpp pthread)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)