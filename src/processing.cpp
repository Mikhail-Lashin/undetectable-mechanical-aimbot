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
                  const cv::Point& aim_center,
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
    for (const auto& contour : out_all_contours) {
        cv::Moments m = cv::moments(contour);
        if (m.m00 != 0) {
            cv::Point current_center(   
                                    static_cast<int>(m.m10 / m.m00),
                                    static_cast<int>(m.m01 / m.m00)
                                    );
            double dist = cv::norm(current_center - aim_center);

            if (dist < min_dist_to_center) {
                min_dist_to_center = dist;
                out_priority_target_pos = current_center;
            }
        }
    }
    return true; // хотя бы одна цель найдена
}

void draw_debug_info(cv::Mat& frame,
                     const cv::Point& aim_center,
                     const std::vector<std::vector<cv::Point>>& all_contours,
                     const cv::Point& priority_target_pos)
{
    // Выделение всех целей
    for (const auto& contour : all_contours) {
        cv::Rect bbox = cv::boundingRect(contour);
        cv::rectangle(frame, bbox, cv::Scalar(0, 255, 0), 1); // рамка
    }

    // Выделение приоритетной цели
    cv::line(frame, aim_center, priority_target_pos, cv::Scalar(255, 255, 255), 1); // линия
}

bool find_crosshair(const cv::Mat& frame, const cv::Mat& crosshair_template, cv::Point& out_center_pos) {
    // Создание матрицы для хранения результатов сравнения
    cv::Mat result;
    int result_cols = frame.cols - crosshair_template.cols + 1;
    int result_rows = frame.rows - crosshair_template.rows + 1;
    result.create(result_rows, result_cols, CV_32FC1);

    // Сопоставление с шаблоном
    // Метод TM_CCOEFF_NORMED устойчив к изменениям яркости
    cv::matchTemplate(frame, crosshair_template, result, cv::TM_CCOEFF_NORMED);

    // Точка с максимальным совпадением
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
    
    if (maxVal >= MATCH_THRESHOLD) {
        // maxLoc - это левый верхний угол найденного шаблона. Нужно пересчитать на центр шаблона:
        out_center_pos.x = maxLoc.x + crosshair_template.cols / 2;
        out_center_pos.y = maxLoc.y + crosshair_template.rows / 2;
        return true;
    }

    return false; // совпадение недостаточно хорошее
}

bool initialize_video_streams(const std::string& input_path, const std::string& output_path,
                              cv::VideoCapture& out_cap, cv::VideoWriter& out_writer) 
{
    out_cap.open(input_path); // откр видео для чтения
    if (!out_cap.isOpened()) {
        std::cout << "Error: Could not open the input video: " << input_path << std::endl;
        return false;
    }

    int frame_width = static_cast<int>(out_cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(out_cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = out_cap.get(cv::CAP_PROP_FPS);
    cv::Size frame_size(frame_width, frame_height);
    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');

    out_writer.open(output_path, fourcc, fps, frame_size); // откр видео для записи
    if (!out_writer.isOpened()) {
        std::cout << "Error: Could not create the output video file: " << output_path << std::endl;
        return false;
    }
    
    return true;
}

bool calibrate_aim_center(cv::VideoCapture& cap, const std::string& template_path, cv::Point& out_aim_center) {
    // загрузка шаблона
    cv::Mat crosshair_template = cv::imread(template_path, cv::IMREAD_GRAYSCALE);
    if (crosshair_template.empty()) {
        std::cout << "Error: Could not open the crosshair template: " << template_path << std::endl;
        return false;
    }

    // считывание первого кадра для поиска
    cv::Mat first_frame;
    cap.read(first_frame);
    if (first_frame.empty()) {
        std::cout << "Error: Video is empty or could not read first frame." << std::endl;
        return false;
    }

    // подготовка кадра к поиску (перевод в Ч/Б)
    cv::Mat first_frame_gray;
    cv::cvtColor(first_frame, first_frame_gray, cv::COLOR_BGR2GRAY);

    // поиск прицела
    if (!find_crosshair(first_frame_gray, crosshair_template, out_aim_center)) {
        std::cout << "Error: Crosshair not found on the first frame. Check template or threshold." << std::endl;
        return false;
    }

    // возврат видео на начало, чтобы основной цикл обработал и первый кадр тоже
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
    
    return true;
}