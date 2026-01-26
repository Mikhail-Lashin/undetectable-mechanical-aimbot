#include "gcodesender.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#include <cerrno>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

GCodeSender::GCodeSender() : sock_fd(-1), connected(false) {}

GCodeSender::~GCodeSender() {
    if (connected && sock_fd >= 0) close(sock_fd);
}

bool GCodeSender::connectToSocket(const std::string& socket_path) {
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Error connecting: " << strerror(errno) << std::endl;
        close(sock_fd);
        sock_fd = -1;
        return false;
    }

    connected = true;
    
    // Устанавливаем таймаут на чтение (1 секунда), чтобы видеть ответы
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    // Читаем приветственное сообщение от Klipper (если оно есть)
    char buffer[4096];
    int len = recv(sock_fd, buffer, sizeof(buffer)-1, 0);
    if (len > 0) {
        buffer[len] = 0;
        std::cout << "[KLIPPER HELLO]: " << buffer << std::endl;
    }

    return true;
}

void GCodeSender::sendCommand(const std::string& command) {
    if (!connected) return;

    // Формируем JSON-RPC
    std::ostringstream ss;
    // ИСПРАВЛЕНИЕ: method теперь "gcode/script"
    ss << "{\"id\": 12345, \"method\": \"gcode/script\", \"params\": {\"script\": \"" 
       << command << "\"}}" 
       << "\x03"; // Разделитель

    std::string packet = ss.str();

    // [DEBUG]
    //std::cout << "[TX] Sending: " << command << " ... ";

    if (send(sock_fd, packet.c_str(), packet.length(), MSG_NOSIGNAL) == -1) {
        std::cerr << "FAILED (Send error)" << std::endl;
        return;
    }

    // [DEBUG] Читаем ответ
    /*char buffer[4096];
    int len = recv(sock_fd, buffer, sizeof(buffer)-1, 0);
    
    if (len > 0) {
        buffer[len] = 0;
        std::cout << "REPLY: " << buffer << std::endl;
    } else {
        std::cout << "NO REPLY (Timeout or empty)" << std::endl;
    }*/
}

void GCodeSender::sendMove(float x_mm, float y_mm, int feedrate_mm_min) {
    std::ostringstream ss;
    ss.precision(3);
    ss << std::fixed << "G1 X" << x_mm << " Y" << y_mm << " F" << feedrate_mm_min;
    sendCommand(ss.str());
}