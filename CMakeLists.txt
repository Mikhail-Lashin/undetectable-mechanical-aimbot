cmake_minimum_required(VERSION 3.10)

project(Circles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc videoio)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(aimbot src/main.cpp)

target_link_libraries(aimbot ${OpenCV_LIBS})