cmake_minimum_required(VERSION 3.10)
project(MatrixSocketLab)

set(CMAKE_CXX_STANDARD 17)

add_executable(server_app Server/main.cpp)
add_executable(client_app Client/main.cpp)

if(WIN32)
    target_link_libraries(server_app ws2_32)
    target_link_libraries(client_app ws2_32)
endif()