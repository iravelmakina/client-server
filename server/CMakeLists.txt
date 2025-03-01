add_executable(server main.cpp Server.cpp)
target_link_libraries(server PRIVATE socket)
target_include_directories(server PRIVATE ../socket)
