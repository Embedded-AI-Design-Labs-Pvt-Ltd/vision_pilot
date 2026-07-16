cmake_minimum_required(VERSION 3.22.1)

find_package(Threads REQUIRED)

add_library(vehicle_interface
        src/can_interface.cpp
        src/file_interface.cpp
)

target_include_directories(vehicle_interface
        PUBLIC
        include
)

target_link_libraries(vehicle_interface
        PUBLIC
        Threads::Threads
)