cmake_minimum_required(VERSION 3.22.1)

find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_library(control_cmd_publisher
    src/cmd_to_ros2.cpp
)

target_include_directories(control_cmd_publisher
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

ament_target_dependencies(control_cmd_publisher PUBLIC rclcpp std_msgs)

target_link_libraries(control_cmd_publisher
    PUBLIC
    rt
)

target_compile_features(control_cmd_publisher PUBLIC cxx_std_17)
