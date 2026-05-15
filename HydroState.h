//init环境参数
#ifndef HYDROSTATE_H
#define HYDROSTATE_H
#include <limits>

//外部环境影响
class Environment {
public:
    double P = 0.0;  // 降水量
    double EM; // 潜在蒸散发量
    double Kc = std::numeric_limits<double>::quiet_NaN(); // 蒸发系数
    double E0 = std::numeric_limits<double>::quiet_NaN(); // 蒸发皿蒸发量

    // 方式1：输入 P 和 EM
    Environment(double p, double em);

    // 方式2：输入 P、E0 和 Kc
    Environment(double p, double e0, double kc);
};

#endif