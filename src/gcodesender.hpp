#pragma once
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

class GCodeSender {
public:
    GCodeSender();
    ~GCodeSender();

    // Подключение к Unix Domain Socket (например, "/tmp/printer")
    bool connectToSocket(const std::string& socket_path);
    
    // Отправка сырой команды
    void sendCommand(const std::string& command);
    
    // Обертка для отправки G1 (движения)
    void sendMove(float x_mm, float y_mm, int feedrate_mm_min);

private:
    int sock_fd;      // Файловый дескриптор сокета
    bool connected;
};