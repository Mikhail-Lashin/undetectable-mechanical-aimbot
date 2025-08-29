#include <opencv2/opencv.hpp>
#include <iostream>

const double MIN_CONTOUR_AREA = 50;
const int H_MIN = 130;
const int S_MIN = 100;
const int V_MIN = 60;
const int H_MAX = 160;
const int S_MAX = 255;
const int V_MAX = 255;
const cv::Scalar LOWER_RANGE_HSV(H_MIN, S_MIN, V_MIN);
const cv::Scalar UPPER_RANGE_HSV(H_MAX, S_MAX, V_MAX);

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
        cv::rectangle(frame, bbox, cv::Scalar(0, 255, 0), 2); // рамка
    }

    // Выделение приоритетной цели
    cv::Point screen_center(frame.cols / 2, frame.rows / 2);
    cv::circle(frame, priority_target_pos, 7, cv::Scalar(0, 0, 255), -1); // круг
    cv::line(frame, screen_center, priority_target_pos, cv::Scalar(255, 0, 0), 2); // линия
}

int main() {
    const std::string input_path = "src/input/input.mp4";
    const std::string output_path = "src/output/output.mp4";
    
    cv::VideoCapture cap(input_path); // открытие видео
    if(!cap.isOpened()) {
        std::cout << "Error: Could not open the input video: " << input_path << std::endl;
        return -1;
    }

    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH)); // свойства видеопотока
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);
    cv::Size frame_size(frame_width, frame_height);
    
    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v'); // FOURCC - код для идентификации кодека.
    cv::VideoWriter writer(output_path, fourcc, fps, frame_size); // объект для записи видео
    if (!writer.isOpened()) {
        std::cout << "Error: Could not create the output video file: " << output_path << std::endl;
        return -1;
    }

    std::cout << "Processing video... Press any key to stop." << std::endl;

    cv::Mat frame;
    while (true) {
        cap.read(frame);
        if (frame.empty()) {
            break;
        }

        cv::Mat hsv_img = BGR_to_HSV(frame);
        cv::Mat bin_img = HSV_to_Binary(hsv_img);

        std::vector<std::vector<cv::Point>> all_targets;
        cv::Point priority_target;
        bool is_targets_found = find_targets(bin_img, all_targets, priority_target);

        cv::Mat result_image = frame.clone();

        if (is_targets_found) {
            draw_debug_info(result_image, all_targets, priority_target);
        }

        writer.write(result_image);
    }

    cap.release();
    writer.release();

    std::cout << "Video processing finished. Output saved to: " << output_path << std::endl;

    return 0;
}