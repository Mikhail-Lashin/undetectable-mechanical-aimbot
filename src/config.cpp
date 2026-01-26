#include "config.hpp"

int H_MIN = 130, S_MIN = 75, V_MIN = 165; // дефолтные значения
int H_MAX = 150, S_MAX = 255, V_MAX = 255;
cv::Point AIM_CENTER(320, 240);
std::string LAPTOP_IP = "192.168.0.230";

std::string getFullConfigPath() {                                           
    return std::string(PROJECT_SOURCE_DIR) + "/config.json"; // абсолютный путь к config.json
}

bool loadConfig(const std::string& path) {
    std::string actual_path = path.empty() ? getFullConfigPath() : path;
    cv::FileStorage fs(actual_path, cv::FileStorage::READ);
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

bool saveConfig(const std::string& path) {
    std::string actual_path = path.empty() ? getFullConfigPath() : path;
    cv::FileStorage fs(actual_path, cv::FileStorage::WRITE);
    
    fs << "H_MIN" << H_MIN;
    fs << "S_MIN" << S_MIN;
    fs << "V_MIN" << V_MIN;
    fs << "H_MAX" << H_MAX;
    fs << "S_MAX" << S_MAX;
    fs << "V_MAX" << V_MAX;
    fs << "AIM_CENTER" << AIM_CENTER;
    fs << "LAPTOP_IP" << LAPTOP_IP;
    fs.release();

    return true;
}