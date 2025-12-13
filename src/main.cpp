#include <iostream>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <chrono> // Для таймера dt

#include "config.hpp"
#include "processing.hpp"
#include "gcodesender.hpp"
#include "pid.hpp" 

int main() {
    // ==========================================
    // 1. ПОДКЛЮЧЕНИЕ К KLIPPER
    // ==========================================
    GCodeSender printer;
    // Проверь путь: ml или m1. Судя по логам сборки, у тебя пользователь "ml"
    const std::string KLIPPER_SOCKET = "/home/ml/printer_data/comms/klippy.sock"; 
    
    std::cout << "--- Connecting to Klipper ---" << std::endl;
    if (printer.connectToSocket(KLIPPER_SOCKET)) {
        std::cout << "SUCCESS: Klipper connected!" << std::endl;
        printer.sendCommand("M17"); // Включить моторы
        printer.sendCommand("G91"); // Относительные координаты
    } else {
        std::cerr << "ERROR: Klipper not connected. Check socket path." << std::endl;
        return -1;
    }

    // ==========================================
    // 2. ПОДКЛЮЧЕНИЕ К КАМЕРЕ (TCP Stream)
    // ==========================================
    std::cout << "--- Connecting to Video Stream ---" << std::endl;
    cv::VideoCapture cap("tcp://127.0.0.1:8888");

    if (!cap.isOpened()) {
        std::cerr << "FATAL: Could not connect to tcp://127.0.0.1:8888" << std::endl;
        return -1;
    }
    std::cout << "SUCCESS: Video stream active!" << std::endl;

    // ==========================================
    // 3. НАСТРОЙКА PID
    // ==========================================
    // P=0.02 (рывок), I=0, D=0.005 (тормоз)
    // Лимиты: +/- 5.0 мм
    PID pidX(0.02f, 0.0f, 0.005f, -5.0f, 5.0f);
    PID pidY(0.02f, 0.0f, 0.005f, -5.0f, 5.0f);

    cv::Point AIM_CENTER(320, 240);
    
    // Переменные для расчета времени (dt)
    auto last_time = std::chrono::high_resolution_clock::now();

    // ==========================================
    // 4. ТАЙМЕР ПЕРЕД СТАРТОМ
    // ==========================================
    std::cout << "!!! WARNING: MOTORS WILL MOVE !!!" << std::endl;
    std::cout << "Starting in 5 seconds..." << std::endl;
    for(int i=5; i>0; i--) {
        std::cout << i << "..." << std::endl;
        sleep(1);
    }
    std::cout << "GO! PID CONTROL ACTIVE!" << std::endl;

    // ==========================================
    // 5. ГЛАВНЫЙ ЦИКЛ
    // ==========================================
    cv::Mat frame;
    int empty_counter = 0;

    while (true) {
        // Расчет времени кадра (dt)
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = current_time - last_time;
        float dt = elapsed.count();
        last_time = current_time;

        // Защита от скачков времени (если fps упал)
        if (dt > 0.1f) dt = 0.1f; 

        // Чтение кадра
        cap.read(frame);
        
        if (frame.empty()) {
            empty_counter++;
            if (empty_counter > 50) {
                 std::cerr << "Lost connection to camera stream!" << std::endl;
                 break;
            }
            continue;
        }
        empty_counter = 0;

        // Обработка CV
        cv::Mat hsv = BGR_to_HSV(frame);
        cv::Mat bin = HSV_to_Binary(hsv);
        
        std::vector<std::vector<cv::Point>> targets;
        cv::Point target_pos;
        
        bool found = find_targets(bin, AIM_CENTER, targets, target_pos);

        if (found) {
            cv::Point error = target_pos - AIM_CENTER;
            double dist = cv::norm(error);

            // Мертвая зона
            if (dist > 5.0) {
                // Расчет PID
                float move_x = pidX.calculate((float)error.x, dt);
                float move_y = pidY.calculate((float)error.y, dt);

                // --- ИНВЕРСИЯ ОСЕЙ ---
                // Восстанавливаем логику из твоего прошлого кода:
                // Было: float move_x = -error.x * k;
                // Значит здесь тоже ставим минус перед результатом PID
                move_x = -move_x;
                move_y = -move_y;

                // Отправка
                printer.sendMove(move_x, move_y, 1000); 
            } else {
                // Сброс PID в мертвой зоне
                pidX.reset();
                pidY.reset();
            }
        } else {
            // Сброс PID при потере цели
            pidX.reset();
            pidY.reset();
        }
    }
    
    printer.sendCommand("M18");
    return 0;
}