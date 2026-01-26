#pragma once 
#include <opencv2/opencv.hpp>
#include <string>

// ОБРАБОТКА ВИДЕОПОТОКА
extern int H_MIN, S_MIN, V_MIN; // переменные из config.json
extern int H_MAX, S_MAX, V_MAX;
extern cv::Point AIM_CENTER;

const cv::Scalar LOWER_RANGE_HSV(H_MIN, S_MIN, V_MIN); // диапазон цветов, соответствующий мишеням (модель HSV)
const cv::Scalar UPPER_RANGE_HSV(H_MAX, S_MAX, V_MAX);

const double MIN_CONTOUR_AREA = 50; // минимальная площадь мишени
const double MATCH_THRESHOLD = 0.8; // порог уверенности для поиска перекрестья прицела по шаблону

// ОТПРАВКА ОТЛАДОЧНОГО ВИДЕО НА НОУТ
extern std::string LAPTOP_IP;
const int LAPTOP_PORT = 9999;

// ПУТЬ К СОКЕТУ KLIPPER
const std::string KLIPPER_SOCKET = "/home/ml/printer_data/comms/klippy.sock";

// PID
const float P_GAIN = 0.03f;
const float I_GAIN = 0.00f;
const float D_GAIN = 0.01f;
const float MAX_MOVE_MM = 5.0f; // максимальный рывок за один кадр

// Функции для работы с config.json
bool loadConfig(const std::string& filename);
void saveConfig(const std::string& filename);