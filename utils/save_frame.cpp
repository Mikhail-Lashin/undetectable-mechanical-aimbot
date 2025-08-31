#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

// Программа сохраняет указанный кадр из видео для создания шаблона перекрестья прицела

int main() {
    const std::string input_video_path = "src/input/input.mp4";
    const std::string output_image_path = "src/output/frame_for_template.png";

    const int frame_number_to_save = 48; // номер кадра для сохранения

    cv::VideoCapture cap(input_video_path);
    if (!cap.isOpened()) {
        std::cerr << "Ошибка: не удалось открыть видеофайл: " << input_video_path << std::endl;
        return -1;
    }

    cap.set(cv::CAP_PROP_POS_FRAMES, frame_number_to_save); // установка видео на нужный кадр

    cv::Mat frame;
    cap.read(frame);

    if (frame.empty()) {
        std::cerr << "Ошибка: не удалось считать кадр номер " << frame_number_to_save
                  << ". Возможно, видео слишком короткое." << std::endl;
        cap.release();
        return -1;
    }

    bool success = cv::imwrite(output_image_path, frame);

    if (success) {
        std::cout << "Кадр успешно сохранен в файл: " << output_image_path << std::endl;
    } else {
        std::cerr << "Ошибка: не удалось сохранить кадр в файл." << std::endl;
        cap.release();
        return -1;
    }

    cap.release();
    
    return 0;
}