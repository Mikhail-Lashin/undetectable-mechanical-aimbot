cmake_minimum_required(VERSION 3.10)

project(Circles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc videoio)

# --- Основное приложение ---
add_executable(aimbot src/main.cpp src/processing.cpp src/gcodesender.cpp src/pid.cpp src/udp_streamer.cpp)
target_include_directories(aimbot PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(aimbot PRIVATE ${OpenCV_LIBS})


# --- Вспомогательные утилиты ---
add_executable(save_frame utils/save_frame.cpp)
target_include_directories(save_frame PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(save_frame PRIVATE ${OpenCV_LIBS})

add_executable(camera_tool utils/camera_tool.cpp src/processing.cpp)
target_include_directories(camera_tool PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(camera_tool PRIVATE ${OpenCV_LIBS})