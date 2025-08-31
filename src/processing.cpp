#include "processing.hpp"
#include "config.hpp"

#include <opencv2/opencv.hpp>
#include <vector>
#include <cfloat> // для доступа к DBL_MAX

cv::Mat BGR_to_HSV(const cv::Mat& bgrImage) {
    cv::Mat hsvImage;
    cv::cvtColor(bgrImage, hsvImage, cv::COLOR_BGR2HSV);
    return hsvImage;
}

cv::Mat HSV_to_Binary(const cv::Mat& hsvImage) {
    cv::Mat binImage;
    cv::inRange(hsvImage, LOWER_RANGE_HSV, UPPER_RANGE_HSV, binImage);

    return binImage;
}

bool find_targets(const cv::Mat& binImage,
                  std::vector<std::vector<cv::Point>>& out_all_contours,
                  cv::Point& out_priority_target_pos)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binImage, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    out_all_contours.clear();
    for (const auto& contour : contours) {
        if (cv::contourArea(contour) > MIN_CONTOUR_AREA) {
            out_all_contours.push_back(contour);
        }
    }

    if (out_all_contours.empty()) {
        return false; // ни одной цели не найдено
    }

    double min_dist_to_center = DBL_MAX;
    cv::Point screen_center(binImage.cols / 2, binImage.rows / 2);
    for (const auto& contour : out_all_contours) {
        cv::Moments m = cv::moments(contour);
        if (m.m00 != 0) {
            cv::Point current_center(   
                                    static_cast<int>(m.m10 / m.m00),
                                    static_cast<int>(m.m01 / m.m00)
                                    );
            double dist = cv::norm(current_center - screen_center);

            if (dist < min_dist_to_center) {
                min_dist_to_center = dist;
                out_priority_target_pos = current_center;
            }
        }
    }
    return true; // хотя бы одна цель найдена
}

void draw_debug_info(cv::Mat& frame,
                     const std::vector<std::vector<cv::Point>>& all_contours,
                     const cv::Point& priority_target_pos)
{
    // Выделение всех целей
    for (const auto& contour : all_contours) {
        cv::Rect bbox = cv::boundingRect(contour);
        cv::rectangle(frame, bbox, cv::Scalar(0, 255, 0), 1); // рамка
    }

    // Выделение приоритетной цели
    cv::Point screen_center(frame.cols / 2, frame.rows / 2);
    cv::line(frame, screen_center, priority_target_pos, cv::Scalar(255, 255, 255), 1); // линия
}
