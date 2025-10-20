cmake_minimum_required(VERSION 3.31)
project(homework03)

set(CMAKE_CXX_STANDARD 20)

add_executable(homework03 main.cpp
        b.h
        error.h
        global.h
        c.h
        d.h
        e.h
        f.h
)

target_link_libraries(homework03 PRIVATE version)
target_link_libraries(homework03 PRIVATE bcrypt)
