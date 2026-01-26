// Утилита для настройки камеры. Пока отлажена только для запуска на винде
// Выход и сохранение конфигурации по cnrtl+C
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <signal.h>                                                         // для сохранения конфига c выходом по cntrl+C

#ifdef _WIN32                                                               // для работы с клавиатурой (стрелки)
   #include <Windows.h>
#endif

#include "../src/processing.hpp"
#include "../src/config.hpp"

std::string winName = "Camera Settings";                                    // подпись окна с настройками
bool keepRunning = true;

void Signal_Handler(int signum){
    // нужна для сохранения конфига c выходом по cntrl+C
    std::cout << "\nInterrupt signal received. Saving and exiting..." << std::endl;
    keepRunning = false;
}
void Import_Config(){
    if (Load_Config()) {
        std::cout << "Config loaded/" << std::endl;
    } else {
        std::cout << "Failed to load config or file not found." << std::endl;
    }
}
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

    // привязка ползунков к переменным из config.hpp
    cv::createTrackbar("H Min", winName, &H_MIN, 179);
    cv::createTrackbar("H Max", winName, &H_MAX, 179);
    cv::createTrackbar("S Min", winName, &S_MIN, 255);
    cv::createTrackbar("S Max", winName, &S_MAX, 255);
    cv::createTrackbar("V Min", winName, &V_MIN, 255);
    cv::createTrackbar("V Max", winName, &V_MAX, 255);
}
void Setup_Aim_Center(){
    #ifdef _WIN32
        int step = 1; // шаг 1 (default) или 10 (SHIFT)
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) step = 10;

        if (GetAsyncKeyState(VK_UP) & 0x8000)    AIM_CENTER.y -=step; 
        if (GetAsyncKeyState(VK_DOWN) & 0x8000)  AIM_CENTER.y += step;
        if (GetAsyncKeyState(VK_LEFT) & 0x8000)  AIM_CENTER.x -= step;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) AIM_CENTER.x += step;
    #endif
}
void Show_Settings_Window(cv::Mat& frame_bgr){
    cv::Mat frame_hsv, mask, mask_bgr, combined;

    cv::cvtColor(frame_bgr, frame_hsv, cv::COLOR_BGR2HSV);
        
    cv::Scalar lower(H_MIN, S_MIN, V_MIN);             
    cv::Scalar upper(H_MAX, S_MAX, V_MAX);
    cv::inRange(frame_hsv, lower, upper, mask);                             // создание бинарного изображения mask (HSV)
    cv::cvtColor(mask, mask_bgr, cv::COLOR_GRAY2BGR);                       // конвертация mask, чтобы склеить с оригиналом

    std::vector<std::vector<cv::Point>> all_targets;
    cv::Point priority_target;

    if (find_targets(mask, AIM_CENTER, all_targets, priority_target)) {
        draw_debug_info(frame_bgr, AIM_CENTER, all_targets, priority_target);
        draw_debug_info(mask_bgr, AIM_CENTER, all_targets, priority_target);
    }

    cv::hconcat(frame_bgr, mask_bgr, combined); 
    cv::imshow(winName, combined);
}
void Export_Config(){
    if (Save_Config()) {
        std::cout << "Settings saved." << std::endl;
    } else {
        std::cout << "Failed to save config." << std::endl;
    }    
}

int main() {
    signal(SIGINT, Signal_Handler);                                         // выполнить Signal_Handler() при нажатии cntrl+C

    cv::VideoCapture cap;                                                   
    if(!Launch_Stream(cap, "192.168.3.21")) return -1;                      // прием видеопотока с rpi
    Import_Config();                                                        // подтягиваем значения из старого конфига
    Setup_UI();                                                             // инициализация интерфейса (ползунки)
    cv::Mat frame_bgr;
    while (keepRunning) {
        if (!cap.read(frame_bgr)) {
            std::cerr << "Frame drop or connection lost..." << std::endl;
            break;
        }
        Show_Settings_Window(frame_bgr);                                    // обработка кадров, вывод окна настроек камеры
        Setup_Aim_Center();                                                 // калибровка начала координат (вручную)
        cv::waitKey(1);                                                     // для работы калибровочных ползунков и пр
    }
    Export_Config();                                                        // сохранение конфига
    cap.release();
    cv::destroyAllWindows();
    return 0;
}