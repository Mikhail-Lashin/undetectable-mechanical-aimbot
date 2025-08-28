#include <opencv2/opencv.hpp>
#include <iostream>

const double MIN_COUNTOUR_AREA = 50;
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

cv::Mat draw_bounding_boxes(const cv::Mat& binImage, const cv::Mat& origImage) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binImage, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    cv::Mat result = origImage.clone();
    
    for (size_t i = 0; i < contours.size(); i++) {
        if (cv::contourArea(contours[i]) > MIN_COUNTOUR_AREA) {
            cv::Rect bbox = cv::boundingRect(contours[i]);
            cv::rectangle(result, bbox, cv::Scalar(0, 255, 0), 2); // Зеленая рамка
        }
    }
    
    return result;
}

int main() {
    const char* inputPath = "../../../src/input/input_rgb.png";
    const char* outputPath = "../../../src/output/output.png";
    
    cv::Mat bgr_img = cv::imread(inputPath, cv::IMREAD_COLOR);
    if(bgr_img.empty()) {
        std::cout << "Could not open or find the image: " << inputPath << std::endl;
        return -1;
    }

    cv::Mat temp_img = BGR_to_HSV(bgr_img);
    temp_img = HSV_to_Binary(temp_img);
    temp_img = draw_bounding_boxes(temp_img, bgr_img);
    
    cv::imwrite(outputPath, temp_img);
    
    return 0;
}