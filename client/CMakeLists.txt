add_executable(client main.cpp Client.cpp)
target_link_libraries(client PRIVATE socket)
target_include_directories(client PRIVATE ../socket)