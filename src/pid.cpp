#include "pid.hpp"

PID::PID(float kp, float ki, float kd, float min_out, float max_out)
    : _kp(kp), _ki(ki), _kd(kd), _min_out(min_out), _max_out(max_out),
      _prev_error(0.0f), _integral(0.0f) {}

float PID::calculate(float error, float dt) {
    if (dt <= 0.0f) return 0.0f; // Защита от деления на ноль

    // 1. Proportional (Пропорциональная часть)
    float P = _kp * error;

    // 2. Integral (Интегральная часть)
    _integral += error * dt;
    // Anti-windup: ограничиваем интеграл, чтобы он не улетел в небеса
    // (примитивный вариант, но рабочий)
    float I = _ki * _integral;

    // 3. Derivative (Дифференциальная часть - скорость изменения ошибки)
    float derivative = (error - _prev_error) / dt;
    float D = _kd * derivative;

    _prev_error = error;

    // Итог
    float output = P + I + D;

    // Ограничение выхода (Clamp)
    return std::clamp(output, _min_out, _max_out);
}

void PID::reset() {
    _prev_error = 0.0f;
    _integral = 0.0f;
}