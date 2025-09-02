#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

#include "config.hpp"
#include "processing.hpp"

int main() {
    const std::string input_path = "src/input/input.mp4";
    const std::string output_path = "src/output/output.mp4";

    cv::VideoCapture cap; // открытие и настройка видеопотока
    cv::VideoWriter writer;
    if (!initialize_video_streams(input_path, output_path, cap, writer)) {
        return -1;
    }
    std::cout << "Video streams initialized successfully." << std::endl;

    const std::string template_path = "src/input/crosshair_template.png"; // поиск начала координат (перекрестье)
    cv::Point AIM_CENTER;
    if (!calibrate_aim_center(cap, template_path, AIM_CENTER)) {
        return -1;
    }

    cv::Mat frame;
    while (true) { // основной цикл
        cap.read(frame);
        if (frame.empty()) {
            break;
        }

        cv::Mat hsv_img = BGR_to_HSV(frame);
        cv::Mat bin_img = HSV_to_Binary(hsv_img);

        std::vector<std::vector<cv::Point>> all_targets;
        cv::Point priority_target;
        bool is_targets_found = find_targets(bin_img, AIM_CENTER, all_targets, priority_target);

        cv::Mat result_image = frame.clone();

        if (is_targets_found) {
            draw_debug_info(result_image, AIM_CENTER, all_targets, priority_target);
        }

        writer.write(result_image);
    }

    cap.release();
    writer.release();

    std::cout << "Video processing finished. Output saved to: " << output_path << std::endl;

    return 0;
}