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

bool find_priority_target(const cv::Mat& binImage, cv::Point& out_target_pos) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binImage, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double min_dist_to_center = DBL_MAX;
    bool target_found = false;
    cv::Point screen_center(binImage.rows / 2, binImage.cols / 2);

    for (const auto& contour : contours) {
        if (cv::contourArea(contour) > MIN_CONTOUR_AREA) {
            cv::Moments m = cv::moments(contour);
            if (m.m00 != 0) {
                cv::Point current_center(   
                                        static_cast<int>(m.m10 / m.m00),
                                        static_cast<int>(m.m01 / m.m00)
                                        );
                double dist = cv::norm(current_center - screen_center);

                if (dist < min_dist_to_center) {
                    min_dist_to_center = dist;
                    out_target_pos = current_center;
                    target_found = true;
                }
            }
        }
    }
    return target_found;
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

        cv::Point target_position;
        bool is_target_found = find_priority_target(bin_img, target_position);

        cv::Mat result_image = frame.clone();

        if (is_target_found) {
            cv::Point screen_center(frame.cols / 2, frame.rows / 2);
            cv::circle(result_image, target_position, 5, cv::Scalar(0, 0, 255), -1);
            cv::line(result_image, screen_center, target_position, cv::Scalar(255, 0, 0), 2);
        }

        writer.write(result_image);
    }

    cap.release();
    writer.release();

    std::cout << "Video processing finished. Output saved to: " << output_path << std::endl;

    return 0;
}