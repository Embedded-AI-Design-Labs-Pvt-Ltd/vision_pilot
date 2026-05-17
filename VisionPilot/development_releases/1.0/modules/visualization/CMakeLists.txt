cmake_minimum_required(VERSION 3.22.1)

find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui videoio)

add_library(visualization
        src/visualization.cpp
        src/visualization_to_rtsp.cpp
)

target_include_directories(visualization
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(visualization
        PUBLIC
        opencv_core
        opencv_imgproc
        opencv_highgui
        opencv_videoio
)