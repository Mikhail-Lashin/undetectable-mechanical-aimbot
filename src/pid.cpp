#include "pid.hpp"
#include <algorithm> // для std::clamp

PID::PID(float kp, float ki, float kd, float min_out, float max_out)
    : _kp(kp), _ki(ki), _kd(kd), _min_out(min_out), _max_out(max_out),
      _prev_error(0.0f), _integral(0.0f), _first_run(true) {}

float PID::calculate(float error, float dt) {
    if (dt <= 0.0f) return 0.0f;

    // 1. Пропорциональная часть
    float P = _kp * error;

    // 2. Интегральная часть с Anti-windup
    _integral += error * dt;
    
    // ОГРАНИЧИВАЕМ интеграл. Он не должен быть больше, чем макс. выход.
    // Это уберет "заторможенность" и перелеты через цель.
    float i_limit = _max_out / (_ki > 0 ? _ki : 1.0f); 
    _integral = std::clamp(_integral, -i_limit, i_limit);
    
    float I = _ki * _integral;

    // 3. Дифференциальная часть с защитой от скачка
    float D = 0.0f;
    if (_first_run) {
        // На первом кадре после появления цели не считаем D, так как нет честной дельты
        _first_run = false;
    } else {
        float derivative = (error - _prev_error) / dt;
        D = _kd * derivative;
    }

    _prev_error = error;

    float output = P + I + D;
    return std::clamp(output, _min_out, _max_out);
}

void PID::reset() {
    _prev_error = 0.0f;
    _integral = 0.0f;
    _first_run = true; // Сбрасываем флаг для защиты от рывка
}