cmake_minimum_required(VERSION 3.31)
project(Homework05)

set(CMAKE_CXX_STANDARD 20)

add_executable(Homework05 WIN32 main.cpp
        helpers.h)

target_link_libraries(Homework05 PRIVATE Wininet comctl32)
