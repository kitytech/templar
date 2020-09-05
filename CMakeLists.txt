cmake_minimum_required(VERSION 3.10)

project(templar)
set(CMAKE_CXX_STANDARD 11)
include_directories($ENV{HOME}/include)
add_executable(example example.cpp)
