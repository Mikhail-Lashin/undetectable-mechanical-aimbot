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

/**
 * @brief Формирует полный абсолютный путь к файлу конфигурации config.json
 * @return std::string Абсолютный путь к файлу в директории исходного кода проекта
 * @note Путь строится на основе макроса PROJECT_SOURCE_DIR, который передается из CMake
 */
std::string Get_Full_Config_Path();

/**
 * @brief Загружает настройки из JSON-файла и обновляет глобальные переменные
 * @param[in] path Путь к файлу конфигурации. Если передан пустая строка, используется путь по умолчанию
 * @return true, если файл успешно открыт и данные прочитаны; false в случае ошибки
 * @note Функция обновляет глобальные параметры: H_MIN/MAX, S_MIN/MAX, V_MIN/MAX, AIM_CENTER и LAPTOP_IP
 * @details Использует cv::FileStorage для парсинга структуры JSON
 */
bool Load_Config(const std::string& path = "");

/**
 * @brief Сохраняет текущие значения глобальных переменных в файл конфигурации
 * @param[in] path Путь для сохранения файла. Если передан пустая строка, используется путь по умолчанию
 * @return true, если запись прошла успешно; false, если не удалось создать или открыть файл на запись
 * @note Формат файла (JSON) определяется автоматически OpenCV по расширению .json
 * @details Записывает текущее состояние всех калибровочных параметров и IP-адрес для связи
 */
bool Save_Config(const std::string& path = "");