#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @brief Конвертирует изображение из BGR в HSV
 * @param[in] bgrImage Входное трехканальное изображение в формате BGR
 * @return cv::Mat Новое изображение в формате HSV
 */
cv::Mat BGR_to_HSV(const cv::Mat& bgrImage);

/**
 * @brief Создает бинарную маску из HSV изображения на основе заданных цветовых порогов
 * @param[in] hsvImage Входное изображение в формате HSV
 * @return cv::Mat Одноканальное бинарное изображение (маска), где белые пиксели соответствуют заданному диапазону цветов
 * @note Цветовые пороги (LOWER_RANGE_HSV, UPPER_RANGE_HSV) определены в config.hpp
 */
cv::Mat HSV_to_Binary(const cv::Mat& hsvImage);

/**
 * @brief Находит все цели на бинарной маске и определяет среди них приоритетную
 * 
 * Функция ищет все контуры на входном бинарном изображении, фильтрует их по
 * минимальной площади, а затем среди отфильрованных находит ту, чей центр
 * масс ближе всего к геометрическому центру изображения
 * 
 * @param[in] binImage Входное одноканальное бинарное изображение (CV_8UC1)
 * @param[in] aim_center Координаты перекрестья прицела
 * @param[out] out_all_contours Вектор, заполненный всеми найденными контурами (целями)
 * @param[out] out_priority_target_pos Переменная, хранящая позицию центра приоритетной цели
 * @return true, если хотя бы одна подходящая цель была найдена, иначе false
 */
bool find_targets(const cv::Mat& binImage,
                  cv::Point& aim_center,
                  std::vector<std::vector<cv::Point>>& out_all_contours,
                  cv::Point& out_priority_target_pos);


/**
 * @brief Рисует отладочную информацию на кадре
 * 
 * Обводит все найденные цели зелеными рамками и рисует линию от центра прицела до приоритетной цели
 * 
 * @param[in,out] frame Изображение, на котором будет производиться отрисовка
 * @param[in] aim_center Координаты перекрестья прицела
 * @param[in] all_contours Вектор со всеми найденными целями для отрисовки
 * @param[in] priority_target_pos Координаты приоритетной цели
 */                  
void draw_debug_info(cv::Mat& frame,
                     cv::Point& aim_center,
                     const std::vector<std::vector<cv::Point>>& all_contours,
                     const cv::Point& priority_target_pos);

/**
 * @brief Находит позицию прицела на изображении методом сопоставления с шаблоном
 * @param[in] frame Кадр, на котором производится поиск
 * @param[in] crosshair_template Изображение-шаблон прицела
 * @param[out] out_center_pos Координаты найденного центра прицела
 * @return true в случае успешного нахождения с достаточной уверенностью, иначе false
 */
bool find_crosshair(const cv::Mat& frame, const cv::Mat& crosshair_template, cv::Point& out_center_pos);

/**
 * @brief Инициализирует объекты для чтения и записи видео.
 * 
 * Открывает входной видеофайл для чтения и создает/открывает выходной
 * видеофайл для записи с теми же параметрами (размер кадра, FPS).
 * 
 * @param[in] input_path Путь к входному видеофайлу.
 * @param[in] output_path Путь к выходному видеофайлу.
 * @param[out] out_cap Объект VideoCapture, который будет инициализирован.
 * @param[out] out_writer Объект VideoWriter, который будет инициализирован.
 * @return true в случае успешной инициализации обоих потоков, иначе false.
 */
bool initialize_video_streams(const std::string& input_path, const std::string& output_path,
                              cv::VideoCapture& out_cap, cv::VideoWriter& out_writer);

/**
 * @brief Калибрует центр прицеливания путем поиска шаблона на первом кадре видео.
 * 
 * Считывает первый кадр из видеопотока, находит на нем изображение прицела
 * с помощью cv::matchTemplate и возвращает его центральные координаты.
 * После выполнения сбрасывает видеопоток на начало.
 * 
 * @param[in,out] cap Инициализированный объект VideoCapture. Будет прочитан и сброшен.
 * @param[in] template_path Путь к файлу с шаблоном прицела.
 * @param[out] out_aim_center Переменная, в которую будут записаны координаты найденного прицела.
 * @return true в случае успешного нахождения прицела, иначе false.
 */
bool calibrate_aim_center(cv::VideoCapture& cap, const std::string& template_path, cv::Point& out_aim_center);

/**
 * @brief Запускает основной цикл покадровой обработки видео.
 * 
 * Функция в цикле считывает кадры из входного потока, выполняет на них
 * поиск целей, рисует отладочную информацию и записывает результат
 * в выходной поток.
 * 
 * @param[in,out] cap Объект VideoCapture для чтения кадров. Его состояние (позиция чтения) будет изменено.
 * @param[in,out] writer Объект VideoWriter для записи обработанных кадров.
 * @param[in] aim_center Константные координаты центра прицела, используемые для расчетов.
 * @note Функция выполняется до тех пор, пока во входном потоке `cap` не закончатся кадры.
 */
void process_video_loop(cv::VideoCapture& cap, cv::VideoWriter& writer, cv::Point& aim_center);