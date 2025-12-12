#include "gcodesender.hpp"
#include <iostream>
#include <cstring>   // memset, strerror
#include <sstream>   // stringstream
#include <cerrno>

GCodeSender::GCodeSender() : sock_fd(-1), connected(false) {}

GCodeSender::~GCodeSender() {
    if (connected && sock_fd >= 0) {
        close(sock_fd);
    }
}

bool GCodeSender::connectToSocket(const std::string& socket_path) {
    // 1. Создаем сокет (AF_UNIX для локальных сокетов, SOCK_STREAM для потока данных)
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    // 2. Настраиваем адрес
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    // Безопасное копирование пути (не более 108 байт для Unix сокетов)
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    // 3. Подключаемся
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Error connecting to Klipper socket at " << socket_path 
                  << ": " << strerror(errno) << std::endl;
        close(sock_fd);
        sock_fd = -1;
        return false;
    }

    connected = true;
    std::cout << "Successfully connected to Klipper socket!" << std::endl;

    // Инициализация режимов
    // M17 - включить моторы
    // G91 - относительные координаты (критично для нашей задачи!)
    sendCommand("M17");
    sendCommand("G91"); 

    return true;
}

void GCodeSender::sendCommand(const std::string& command) {
    if (!connected) return;

    std::string final_cmd = command;
    // Klipper обязательно ждет символ новой строки
    if (final_cmd.empty() || final_cmd.back() != '\n') {
        final_cmd += "\n";
    }

    // Отправляем данные
    // MSG_NOSIGNAL предотвращает падение программы, если сокет разорвался (EPIPE)
    ssize_t sent_bytes = send(sock_fd, final_cmd.c_str(), final_cmd.length(), MSG_NOSIGNAL);

    if (sent_bytes == -1) {
        std::cerr << "Error sending data: " << strerror(errno) << std::endl;
        // Можно добавить логику переподключения здесь
        connected = false;
        close(sock_fd);
    }
}

void GCodeSender::sendMove(float x_mm, float y_mm, int feedrate_mm_min) {
    // Формируем строку G-кода
    // G1 - линейная интерполяция
    std::ostringstream ss;
    ss << "G1 X" << x_mm << " Y" << y_mm << " F" << feedrate_mm_min;
    
    sendCommand(ss.str());
}