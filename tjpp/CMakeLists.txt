cmake_minimum_required(VERSION 3.5)
project(tjpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -DTIMING__")

include_directories(/opt/libjpeg-turbo/include include dep/syncqueue)
link_directories(/opt/libjpeg-turbo/lib)
link_libraries(turbojpeg)
add_executable(comp-decomp test/uncompress-compress.cpp)