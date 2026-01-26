#include <iostream>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <chrono>

#include "config.hpp"
#include "processing.hpp"
#include "gcodesender.hpp"
#include "pid.hpp"
#include "udp_streamer.hpp" 

void Import_Config(){
    if (Load_Config()) {
        std::cout << "Config loaded successfully!" << std::endl;
    } else {
        std::cerr << "Warning: config.json not found, using defaults." << std::endl;
    }
}
void Start_Klipper(GCodeSender& printer){
    std::cout << ">>> Connecting to Klipper" << std::endl;
    
    if (printer.connectToSocket(KLIPPER_SOCKET)) {
        std::cout << "    SUCCESS: Klipper connected!" << std::endl;
        printer.sendCommand("M17"); // Включить моторы
        printer.sendCommand("G91"); // Относительные координаты
    } else {
        std::cerr << "    FATAL: Klipper not connected. Check socket path." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
void Start_Stream(){
    std::string cmd = "rpicam-vid -t 0 --inline --listen -o tcp://0.0.0.0:8888 "
                      "--width 640 --height 480 --framerate 30 --codec mjpeg > /dev/null 2>&1 &";
                      // "> /dev/null 2>&1" - чтобы логи камеры не выводились в консоль
                      // " & " - чтобы запустить процесс в фоновом режиме и вернуть управление основной программе
    std::system(cmd.c_str());

    std::cout << ">>> Camera initializing..." << std::endl;
    sleep(2);
}
void Check_Stream(cv::VideoCapture& cap){
    std::cout << ">>> Connecting to Camera Stream..." << std::endl;

    if (!cap.isOpened()) {
        std::cerr << "    FATAL: Could not connect to tcp://127.0.0.1:8888" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "    SUCCESS: Video stream active!" << std::endl;
}
void Countdown(int n_seconds){
    std::cout << "!!! WARNING: START IN 5 SECONDS !!!" << std::endl;
        for(int i=n_seconds; i>0; i--) {
            std::cout << i << "..." << std::endl;
            sleep(1);
        }
        std::cout << "GO!" << std::endl;
}

int main() {
    Import_Config();
    
    GCodeSender printer;
    Start_Klipper(printer);

    Start_Stream();
    cv::VideoCapture cap("tcp://127.0.0.1:8888");
    Check_Stream(cap);                                                  // считывает видео с камеры на rpi

    std::cout << ">>> Initializing PID & Streamer" << std::endl;
    PID pidX(P_GAIN, I_GAIN, D_GAIN, -MAX_MOVE_MM, MAX_MOVE_MM);        // PID для X и Y
    PID pidY(P_GAIN, I_GAIN, D_GAIN, -MAX_MOVE_MM, MAX_MOVE_MM);

    UdpStreamer streamer(LAPTOP_IP, LAPTOP_PORT);                       // стример для отладки
    auto last_time = std::chrono::high_resolution_clock::now();         // переменные времени

    Countdown(5);                                                       // обратный отсчет

    // ГЛАВНЫЙ ЦИКЛ
    cv::Mat frame;
    cv::Mat debug_frame;                                                // кадр для отправки на ноут
    int frame_counter = 0, empty_counter = 0;

    while (true) {
        // расчет времени кадра dt
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = current_time - last_time;
        float dt = elapsed.count();
        if (dt > 0.1f) dt = 0.1f; // Защита от скачков времени
        last_time = current_time;

        // чтение кадра
        // Сливаем старые кадры из буфера, чтобы получить самый актуальный
        for(int i = 0; i < 5; i++) {
            cap.grab(); // Быстро захватывает, но не декодирует кадр
        }
        cap.retrieve(frame); // Декодируем только последний
        if (frame.empty()) {
            empty_counter++;
            if (empty_counter > 50) {
                 std::cerr << "Lost connection to camera stream!" << std::endl;
                 break;
            }
            continue;
        }
        empty_counter = 0;

        // подготовка отладочного кадра
        debug_frame = frame.clone();
        cv::Mat hsv = BGR_to_HSV(frame);
        cv::Mat bin = HSV_to_Binary(hsv);
        
        std::vector<std::vector<cv::Point>> targets;
        cv::Point target_pos;
        if (find_targets(bin, AIM_CENTER, targets, target_pos)) {
            // Вектор ошибки (в пикселях)
            cv::Point error = target_pos - AIM_CENTER;
            double dist = cv::norm(error);

            // Рисуем на отладочном кадре
            cv::rectangle(debug_frame, cv::Rect(target_pos.x-10, target_pos.y-10, 20, 20), cv::Scalar(0, 255, 0), 2);
            cv::line(debug_frame, AIM_CENTER, target_pos, cv::Scalar(0, 0, 255), 1);

            if (dist > 5.0) { // Мертвая зона 5 пикселей
                float move_y = pidX.calculate((float)error.x, dt);
                float move_x = pidY.calculate((float)error.y, dt);

                printer.sendMove(move_x, move_y, 300); 

            } else {
                // Если мы в центре - сбрасываем PID, чтобы не накапливалась ошибка
                pidX.reset();
                pidY.reset();
            }
        } else {
            // Цель потеряна - сброс
            pidX.reset();
            pidY.reset();
        }

        // Рисуем прицел
        cv::circle(debug_frame, AIM_CENTER, 4, cv::Scalar(255, 0, 0), -1);

        // Отправка видео на 
        frame_counter++;
        // Отправляем каждый 4-й кадр (~7 FPS на ноуте), чтобы не грузить сеть
        /*if (frame_counter % 4 == 0) {
            streamer.sendFrame(debug_frame);
        }*/
       streamer.sendFrame(debug_frame);
    }
    printer.sendCommand("M18"); // Выключение моторов при выходе
    return 0;
}