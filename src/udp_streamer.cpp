#include "udp_streamer.hpp"
#include <iostream>
#include <vector>
#include <unistd.h>

UdpStreamer::UdpStreamer(const std::string& dest_ip, int dest_port) {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP сокет
    if (sock_fd < 0) {
        std::cerr << "UDP Streamer: Failed to create socket" << std::endl;
        return;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip.c_str(), &dest_addr.sin_addr);
}

UdpStreamer::~UdpStreamer() {
    if (sock_fd >= 0) close(sock_fd);
}

void UdpStreamer::sendFrame(const cv::Mat& frame) {
    if (sock_fd < 0 || frame.empty()) return;

    // 1. Сжимаем в JPEG (качество 50% для скорости)
    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 50};
    cv::imencode(".jpg", frame, buffer, params);

    // 2. UDP имеет лимит пакета ~65 КБ.
    // Если картинка больше, мы ее не отправим (для простоты, без фрагментации)
    if (buffer.size() > 60000) {
        // std::cerr << "Frame too big for UDP!" << std::endl; 
        return;
    }

    // 3. Отправляем
    sendto(sock_fd, buffer.data(), buffer.size(), 0, 
           (struct sockaddr*)&dest_addr, sizeof(dest_addr));
}