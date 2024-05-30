cmake_minimum_required(VERSION 3.22)
project(sylar LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

# set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function")
file(GLOB_RECURSE LIB_SRC "${CMAKE_SOURCE_DIR}/sylar/*")
include_directories(${CMAKE_SOURCE_DIR})
add_library(sylar SHARED ${LIB_SRC})

file(GLOB SOURCE_FILES "${CMAKE_SOURCE_DIR}/test/*")
set(LINK_LIBS sylar dl yaml-cpp pthread)

foreach(SOURCE_FILE ${SOURCE_FILES})
    get_filename_component(FILE_NAME ${SOURCE_FILE} NAME_WE)
    add_executable(${FILE_NAME} ${SOURCE_FILE})
    target_link_libraries(${FILE_NAME} ${LINK_LIBS})
endforeach()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
