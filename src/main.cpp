#include <iostream>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <chrono>

#include "config.hpp"
#include "processing.hpp"
#include "gcodesender.hpp"
#include "pid.hpp"
#include "udp_streamer.hpp" 

int main() {
    const std::string LAPTOP_IP = "192.168.0.230"; // для отправки отладочного видеопотока на ноут
    const int LAPTOP_PORT = 9999;

    const std::string KLIPPER_SOCKET = "/home/ml/printer_data/comms/klippy.sock"; // для передачи g-кода RPi -> stm

    // Настройки PID
    float P_GAIN = 0.03f;
    float D_GAIN = 0.01f;
    float MAX_MOVE_MM = 5.0f; // максимальный рывок за один кадр

    // ПОДКЛЮЧЕНИЕ К KLIPPER
    GCodeSender printer;
    std::cout << "--- [1/4] Connecting to Klipper ---" << std::endl;
    
    if (printer.connectToSocket(KLIPPER_SOCKET)) {
        std::cout << "SUCCESS: Klipper connected!" << std::endl;
        printer.sendCommand("M17"); // Включить моторы
        printer.sendCommand("G91"); // Относительные координаты
    } else {
        std::cerr << "FATAL: Klipper not connected. Check socket path." << std::endl;
        return -1;
    }

    // ПОДКЛЮЧЕНИЕ К ВИДЕОПОТОКУ
    std::cout << "--- [2/4] Connecting to Camera Stream ---" << std::endl;
    // Читаем поток от rpicam-vid (запущенного во втором окне)
    cv::VideoCapture cap("tcp://127.0.0.1:8888");

    if (!cap.isOpened()) {
        std::cerr << "FATAL: Could not connect to tcp://127.0.0.1:8888" << std::endl;
        std::cerr << "Run 'rpicam-vid' in another terminal first!" << std::endl;
        return -1;
    }
    std::cout << "SUCCESS: Video stream active!" << std::endl;

    // ИНИЦИАЛИЗАЦИЯ КОНТРОЛЛЕРОВ
    std::cout << "--- [3/4] Initializing PID & Streamer ---" << std::endl;
    
    // PID регуляторы для осей X и Y
    PID pidX(P_GAIN, 0.0f, D_GAIN, -MAX_MOVE_MM, MAX_MOVE_MM);
    PID pidY(P_GAIN, 0.0f, D_GAIN, -MAX_MOVE_MM, MAX_MOVE_MM);

    // стример для отладки
    UdpStreamer streamer(LAPTOP_IP, LAPTOP_PORT);

    // Центр прицеливания (середина разрешения 640x480)
    cv::Point AIM_CENTER(320, 240);
    
    // Переменные времени
    auto last_time = std::chrono::high_resolution_clock::now();

    // ОБРАТНЫЙ ОТСЧЕТ
    std::cout << "!!! WARNING: MOTORS WILL MOVE IN 5 SECONDS !!!" << std::endl;
    for(int i=5; i>0; i--) {
        std::cout << i << "..." << std::endl;
        sleep(1);
    }
    std::cout << "GO! TRACKING ACTIVE!" << std::endl;

    // ГЛАВНЫЙ ЦИКЛ
    cv::Mat frame;
    cv::Mat debug_frame; // Кадр для отправки на ноут
    int frame_counter = 0;
    int empty_counter = 0;

    while (true) {
        // --- 5.1 Расчет DT (времени кадра) ---
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = current_time - last_time;
        float dt = elapsed.count();
        last_time = current_time;

        // Защита от скачков времени (если fps упал, PID не должен сойти с ума)
        if (dt > 0.1f) dt = 0.1f; 

        // --- 5.2 Чтение кадра ---
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

        // --- 5.3 Подготовка отладочного кадра ---
        // Клонируем оригинал, чтобы рисовать на нем зеленые квадратики
        debug_frame = frame.clone();

        // --- 5.4 Компьютерное зрение ---
        cv::Mat hsv = BGR_to_HSV(frame);
        cv::Mat bin = HSV_to_Binary(hsv);
        
        std::vector<std::vector<cv::Point>> targets;
        cv::Point target_pos;
        
        bool found = find_targets(bin, AIM_CENTER, targets, target_pos);

        // --- 5.5 Логика управления ---
        if (found) {
            // Вектор ошибки (в пикселях)
            cv::Point error = target_pos - AIM_CENTER;
            double dist = cv::norm(error);

            // Рисуем на отладочном кадре
            cv::rectangle(debug_frame, cv::Rect(target_pos.x-10, target_pos.y-10, 20, 20), cv::Scalar(0, 255, 0), 2);
            cv::line(debug_frame, AIM_CENTER, target_pos, cv::Scalar(0, 0, 255), 1);

            // Мертвая зона (5 пикселей)
            if (dist > 5.0) {
                // Считаем PID
                float move_x = pidX.calculate((float)error.x, dt);
                float move_y = pidY.calculate((float)error.y, dt);

                // --- ИНВЕРСИЯ И ПЕРЕПУТАННЫЕ ОСИ ---
                // Здесь мы приводим логику камеры к логике стола.
                // 1. Инверсия (стол едет влево, чтобы мышь поехала вправо) -> Ставим минусы.
                move_x = -move_x;
                // move_y = -move_y;

                // 2. Если оси перепутаны (X едет по Y), меняем аргументы в sendMove:
                // Было: printer.sendMove(move_x, move_y, 6000);
                // Стало (если нужно): printer.sendMove(move_y, move_x, 6000);
                
                // Стандартная отправка:
                printer.sendMove(move_x, move_y, 6000); 

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

        // Рисуем прицел (центр экрана)
        cv::circle(debug_frame, AIM_CENTER, 4, cv::Scalar(255, 0, 0), -1);

        // --- 5.6 Отправка видео на ноут ---
        frame_counter++;
        // Отправляем каждый 4-й кадр (~7 FPS на ноуте), чтобы не грузить сеть
        if (frame_counter % 4 == 0) {
            streamer.sendFrame(debug_frame);
        }
    }
    
    // Выключение моторов при выходе
    printer.sendCommand("M18");
    return 0;
}