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
 * @param[out] out_all_contours Вектор, заполненный всеми найденными контурами (целями)
 * @param[out] out_priority_target_pos Переменная, хранящая позицию центра приоритетной цели
 * @return true, если хотя бы одна подходящая цель была найдена, иначе false
 */
bool find_targets(const cv::Mat& binImage,
                  std::vector<std::vector<cv::Point>>& out_all_contours,
                  cv::Point& out_priority_target_pos);


/**
 * @brief Рисует отладочную информацию на кадре
 * 
 * Обводит все найденные
 * цели зелеными рамками и рисует линию от центра прицела до приоритетной цели
 * 
 * @param[in,out] frame Изображение, на котором будет производиться отрисовка
 * @param[in] all_contours Вектор со всеми найденными целями для отрисовки
 * @param[in] priority_target_pos Координаты приоритетной цели
 * @param[in] aim_center Координаты точки прицеливания (центра экрана)
 */                  
void draw_debug_info(cv::Mat& frame,
                     const std::vector<std::vector<cv::Point>>& all_contours,
                     const cv::Point& priority_target_pos);