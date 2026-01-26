#include "config.hpp"

int H_MIN = 130, S_MIN = 75, V_MIN = 165; // дефолтные значения
int H_MAX = 150, S_MAX = 255, V_MAX = 255;
cv::Point AIM_CENTER(320, 240);
std::string LAPTOP_IP = "192.168.0.230";

bool loadConfig(const std::string& filename) {
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) return false;

    fs["H_MIN"] >> H_MIN;
    fs["S_MIN"] >> S_MIN;
    fs["V_MIN"] >> V_MIN;
    fs["H_MAX"] >> H_MAX;
    fs["S_MAX"] >> S_MAX;
    fs["V_MAX"] >> V_MAX;
    fs["AIM_CENTER"] >> AIM_CENTER;
    fs["LAPTOP_IP"] >> LAPTOP_IP;

    fs.release();
    return true;
}

void saveConfig(const std::string& filename) {
    cv::FileStorage fs(filename, cv::FileStorage::WRITE); // OpenCV определяет формат .json по расширению файла автоматически
    
    fs << "H_MIN" << H_MIN;
    fs << "S_MIN" << S_MIN;
    fs << "V_MIN" << V_MIN;
    fs << "H_MAX" << H_MAX;
    fs << "S_MAX" << S_MAX;
    fs << "V_MAX" << V_MAX;
    fs << "AIM_CENTER" << AIM_CENTER;
    fs << "LAPTOP_IP" << LAPTOP_IP;

    fs.release();
}