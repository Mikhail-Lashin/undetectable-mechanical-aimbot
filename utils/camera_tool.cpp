// Утилита для настройки камеры. Пока отлажена только для запуска на винде
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

#ifdef _WIN32                                                               // для работы с клавиатурой (стрелки)
    #include <Windows.h>
#endif

#include "../src/processing.hpp"

std::string winName = "Camera Settings";                                    // глобальная подпись интерфейса

struct HSVConfig {
    int h_min = 0, h_max = 179;
    int s_min = 0, s_max = 255;
    int v_min = 0, v_max = 255;
} config;

bool Launch_Stream(cv::VideoCapture& cap, const std::string& rpi_ip){ 
    // Пример команды для запуска стрима на малинке:
    // rpicam-vid -t 0 --inline --listen -o tcp://0.0.0.0:8888 --width 640 --height 480 --framerate 30 --codec mjpeg
    std::string stream_url = "tcp://" + rpi_ip + ":8888";
    std::cout << "Connecting to: " << stream_url << "..." << std::endl;

    cap.open(stream_url);
    if (!cap.isOpened()) {
        std::cerr << "ERROR: Could not open video stream!" << std::endl;
        std::cerr << "Check if 'rpicam-vid' is running on Raspberry Pi." << std::endl;
        return false;
    }
    std::cout << "SUCCESS: Connected to stream." << std::endl;
    return true;
}
void Setup_UI(){
    cv::namedWindow(winName, cv::WINDOW_NORMAL);

    cv::createTrackbar("H Min", winName, &config.h_min, 179);
    cv::createTrackbar("H Max", winName, &config.h_max, 179);
    cv::createTrackbar("S Min", winName, &config.s_min, 255);
    cv::createTrackbar("S Max", winName, &config.s_max, 255);
    cv::createTrackbar("V Min", winName, &config.v_min, 255);
    cv::createTrackbar("V Max", winName, &config.v_max, 255);
}
void Setup_Aim_Center(cv::Point& aim_center){
    #ifdef _WIN32
        int step = 1; // шаг 1 (default) или 10 (SHIFT)
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) step = 10;

        if (GetAsyncKeyState(VK_UP) & 0x8000)    aim_center.y -=step; 
        if (GetAsyncKeyState(VK_DOWN) & 0x8000)  aim_center.y += step;
        if (GetAsyncKeyState(VK_LEFT) & 0x8000)  aim_center.x -= step;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) aim_center.x += step;
    #endif
}
void Show_Settings_Window(cv::Mat& frame_bgr, cv::Point& aim_center){
    cv::Mat frame_hsv, mask, mask_bgr, combined;

    cv::cvtColor(frame_bgr, frame_hsv, cv::COLOR_BGR2HSV);
        
    cv::Scalar lower(config.h_min, config.s_min, config.v_min);             
    cv::Scalar upper(config.h_max, config.s_max, config.v_max);
    cv::inRange(frame_hsv, lower, upper, mask);                             // создание бинарного изображения mask (HSV)
    cv::cvtColor(mask, mask_bgr, cv::COLOR_GRAY2BGR);                       // конвертация mask, чтобы склеить с оригиналом

    std::vector<std::vector<cv::Point>> all_targets;
    cv::Point priority_target;

    if (find_targets(mask, aim_center, all_targets, priority_target)) {
        draw_debug_info(frame_bgr, aim_center, all_targets, priority_target);
        draw_debug_info(mask_bgr, aim_center, all_targets, priority_target);
    }

    cv::hconcat(frame_bgr, mask_bgr, combined); 
    cv::imshow(winName, combined);
}

int main() {
    cv::VideoCapture cap;                                                   // прием видеопотока с rpi
    if(!Launch_Stream(cap, "192.168.3.21")) return -1;

    Setup_UI();                                                             // инициализация интерфейса (ползунки)

    cv::Mat frame_bgr;
    cv::Point aim_center(0,0);
    while (true) {                                                          // главный цикл
        if (!cap.read(frame_bgr)) {
            std::cerr << "Frame drop or connection lost..." << std::endl;
            break;
        }

        Show_Settings_Window(frame_bgr, aim_center);                        // обработка кадров, вывод окна настроек камеры

        cv::waitKey(1);                                                     // считывание клавиши
        Setup_Aim_Center(aim_center);                                       // калибровка начала координат (вручную)
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}