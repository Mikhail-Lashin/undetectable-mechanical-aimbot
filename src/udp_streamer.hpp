#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>

class UdpStreamer {
public:
    UdpStreamer(const std::string& dest_ip, int dest_port);
    ~UdpStreamer();

    // Отправляет кадр
    void sendFrame(const cv::Mat& frame);

private:
    int sock_fd;
    struct sockaddr_in dest_addr;
};