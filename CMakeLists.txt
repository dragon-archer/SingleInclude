cmake_minimum_required(VERSION 3.16)
project(singleinclude VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(SRC main.cpp)
add_executable(singleinclude ${SRC})
if(DEFINED WITH_FMTLIB)
message(STATUS "Use user defined fmtlib")
target_include_directories(singleinclude PRIVATE ${WITH_FMTLIB})
endif()

