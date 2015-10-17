cmake_minimum_required(VERSION 3.3)
project(judger)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        myexec.cpp syscall_checker.c)

add_executable(judger ${SOURCE_FILES})