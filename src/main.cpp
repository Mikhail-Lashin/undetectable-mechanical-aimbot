#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

#include "config.hpp"
#include "processing.hpp"

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