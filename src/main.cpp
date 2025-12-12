#include <iostream>
#include <opencv2/opencv.hpp>
#include <unistd.h>

#include "config.hpp"
#include "processing.hpp"
#include "gcodesender.hpp"

int main() {
    // ==========================================
    // 1. ПОДКЛЮЧЕНИЕ К KLIPPER
    // ==========================================
    GCodeSender printer;
    // Твой рабочий путь
    const std::string KLIPPER_SOCKET = "/home/ml/printer_data/comms/klippy.sock"; 
    
    std::cout << "--- Connecting to Klipper ---" << std::endl;
    if (printer.connectToSocket(KLIPPER_SOCKET)) {
        std::cout << "SUCCESS: Klipper connected!" << std::endl;
        // На всякий случай дублируем настройки движения
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
    // Убедись, что rpicam-vid запущен во втором окне!
    cv::VideoCapture cap("tcp://127.0.0.1:8888");

    if (!cap.isOpened()) {
        std::cerr << "FATAL: Could not connect to tcp://127.0.0.1:8888" << std::endl;
        std::cerr << "Did you run: rpicam-vid -t 0 --inline --listen -o tcp://0.0.0.0:8888 --width 640 --height 480 --framerate 30 --codec mjpeg ?" << std::endl;
        return -1;
    }
    std::cout << "SUCCESS: Video stream active!" << std::endl;

    // ==========================================
    // 3. ПОДГОТОВКА
    // ==========================================
    cv::Point AIM_CENTER(320, 240); // Центр экрана (прицел)
    
    std::cout << "!!! WARNING: MOTORS WILL MOVE !!!" << std::endl;
    std::cout << "Make sure you homed the printer (G28) in web interface!" << std::endl;
    std::cout << "Starting in 5 seconds..." << std::endl;
    
    for(int i=5; i>0; i--) {
        std::cout << i << "..." << std::endl;
        sleep(1);
    }
    std::cout << "GO! TRACKING ACTIVE!" << std::endl;

    // ==========================================
    // 4. ГЛАВНЫЙ ЦИКЛ (LOOP)
    // ==========================================
    cv::Mat frame;
    int empty_counter = 0;

    while (true) {
        // 4.1 Чтение кадра
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

        // 4.2 Обработка зрения
        cv::Mat hsv = BGR_to_HSV(frame);
        cv::Mat bin = HSV_to_Binary(hsv);
        
        std::vector<std::vector<cv::Point>> targets;
        cv::Point target_pos;
        
        bool found = find_targets(bin, AIM_CENTER, targets, target_pos);

        // 4.3 Логика движения
        if (found) {
            // Вектор ошибки: от Центра Прицела к Цели
            cv::Point error = target_pos - AIM_CENTER;
            
            // Дистанция до цели
            double dist = cv::norm(error);

            // Мертвая зона (Deadzone) - чтобы не дрожать, когда цель уже почти в центре
            if (dist > 5.0) {
                // --- НАСТРОЙКА КОЭФФИЦИЕНТОВ ---
                // k = 0.05 означает: при ошибке 100 пикселей сдвинуть стол на 5 мм.
                float k = 0.001; 
                
                // --- ФИЗИКА ИНВЕРСИИ ---
                // Если цель СПРАВА (error.x > 0), мышь надо сдвинуть ВПРАВО.
                // Чтобы мышь сдвинулась вправо относительно стола, СТОЛ должен поехать ВЛЕВО.
                // Поэтому знак МИНУС.
                float move_x = -error.x * k;
                float move_y = -error.y * k; 

                // --- CLAMP (Ограничитель рывка) ---
                // Защита механики: не даем команду больше 5 мм за один цикл (~30 мс)
                float max_step = 5.0;
                if (move_x > max_step) move_x = max_step;
                if (move_x < -max_step) move_x = -max_step;
                if (move_y > max_step) move_y = max_step;
                if (move_y < -max_step) move_y = -max_step;

                // --- ОТПРАВКА ---
                // F3000 = 50 мм/сек. Можно поднять до F6000 или F12000 для резкости.
                printer.sendMove(move_x, move_y, 6000); 
                
                // Дебаг: показываем, куда едем.
                // Если консоль тормозит, закомментируй эту строку.
                // std::cout << "Err: " << error << " -> Move: " << move_x << ", " << move_y << std::endl;
            }
        }
    }
    
    // Отключаем моторы при выходе (если цикл прервется)
    printer.sendCommand("M18");
    return 0;
}