cmake_minimum_required(VERSION 3.29)
project(client-server)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(socket)
add_subdirectory(client)
add_subdirectory(server)
