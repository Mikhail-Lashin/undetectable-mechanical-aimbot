#pragma once
#include <algorithm> // для std::clamp

class PID {
public:
    // Конструктор: коэффициенты P, I, D и лимиты выхода
    PID(float kp, float ki, float kd, float min_out, float max_out);

    // Главная функция расчета
    float calculate(float error, float dt);

    // Сброс (полезно при смене цели)
    void reset();

private:
    float _kp, _ki, _kd;
    float _min_out, _max_out;
    float _prev_error;
    float _integral;
};