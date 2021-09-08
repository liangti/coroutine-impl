cmake_minimum_required(VERSION 3.6)

project(cpp-coroutine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    cpp-coroutine
    src/coroutine.cpp
)

target_include_directories(
    cpp-coroutine
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)

include(CTest)

set(CMAKE_ROOT_DIR ${CMAKE_SOURCE_DIR})
if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(t)
endif()