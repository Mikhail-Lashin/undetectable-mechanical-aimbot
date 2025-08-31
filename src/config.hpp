#pragma once 
#include <opencv2/opencv.hpp>

const double MIN_CONTOUR_AREA = 50; // минимальная площадь мишени

const int H_MIN = 130;
const int S_MIN = 100;
const int V_MIN = 60;
const int H_MAX = 160;
const int S_MAX = 255;
const int V_MAX = 255;

const cv::Scalar LOWER_RANGE_HSV(H_MIN, S_MIN, V_MIN); // диапазон цветов, соответствующий мишеням (модель HSV)
const cv::Scalar UPPER_RANGE_HSV(H_MAX, S_MAX, V_MAX);