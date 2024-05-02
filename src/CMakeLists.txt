cmake_minimum_required(VERSION 3.10)

project(TCP LANGUAGES C CXX)

include_directories(start)

add_executable(server 
    server.cc
    start/main_server.cc
)

add_executable(client 
    client.cc
    start/main_client.cc
)