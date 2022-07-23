cmake_minimum_required(VERSION 3.16)
project(singleinclude VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(SRC main.cpp)
add_executable(singleinclude ${SRC})

