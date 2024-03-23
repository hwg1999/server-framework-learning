cmake_minimum_required(VERSION 3.22)
project(sylar)

# set(CMAKE_CXX_FLAGS "-g -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC sylar/log.cpp sylar/util.cpp)

add_library(sylar SHARED ${LIB_SRC})
include_directories(${CMAKE_SOURCE_DIR}/sylar)

add_executable(test test/log_test.cpp )
add_dependencies(test sylar)
target_link_libraries(test sylar)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)