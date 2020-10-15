cmake_minimum_required(VERSION 3.5)
project(wsjs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto")

include_directories(dep/websocketplus/include
                    /usr/local/libwebsockets/include
                    dep/serializer/include
                    /opt/local/include)
link_directories(/usr/local/libwebsockets/lib)
link_libraries(websockets)

add_executable(appclienttest appclient-test/appclient-test.cpp)
