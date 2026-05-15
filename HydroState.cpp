// Copyright (C) 2026 廖振杰(Liao Zhenjie), Licensed under GNU GPL v2.0
#include "HydroState.h"
#include <stdexcept>

//初始化与验证
Environment::Environment(double p, double em)
    : P(p), EM(em), Kc(std::numeric_limits<double>::quiet_NaN()), E0(std::numeric_limits<double>::quiet_NaN()) {
    if (p < 0.0) {
        throw std::invalid_argument("降雨量 P 必须为非负数！");
    }
    if (em < 0.0) {
        throw std::invalid_argument("潜在蒸散发量 EM 必须为正数！");
    }
}
Environment::Environment(double p, double e0, double kc)
    : P(p), EM(kc* e0), Kc(kc), E0(e0) {
    if (p < 0.0) {
        throw std::invalid_argument("降雨量 P 必须为非负数！");
    }
    if (e0 <= 0.0) {
        throw std::invalid_argument("蒸发皿蒸发量 E0 必须为正数！");
    }
    if (kc <= 0.0 || kc > 1.0) {
        throw std::invalid_argument("蒸发系数 Kc 必须在 0 和 1 之间！");
    }
}