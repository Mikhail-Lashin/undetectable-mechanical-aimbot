cmake_minimum_required(VERSION 3.10)

project(Circles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc videoio)

# --- Основное приложение ---
add_executable(aimbot src/main.cpp src/processing.cpp)
# Пути для заголовочников и библиотеки OpenCV ТОЛЬКО к цели aimbot
target_include_directories(aimbot PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(aimbot PRIVATE ${OpenCV_LIBS})


# --- Вспомогательная утилита ---
add_executable(save_frame utils/save_frame.cpp)
# Пути для заголовочников и библиотек OpenCV ТОЛЬКО к цели save_frame
target_include_directories(save_frame PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(save_frame PRIVATE ${OpenCV_LIBS})