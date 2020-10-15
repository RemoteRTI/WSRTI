cmake_minimum_required(VERSION 3.5)
project(serialize)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_executable(serialization-test test/SerializerTest.cpp)
target_include_directories(serialization-test PRIVATE include)

