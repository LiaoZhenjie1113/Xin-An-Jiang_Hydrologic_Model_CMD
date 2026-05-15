#ifndef EVAPORATION_H
#define EVAPORATION_H
#include <string>
#include "HydroState.h"

// 土壤状态
class SoilState {
private:
    const double C;   // 深层蒸散发系数（只读）
    const double WUM; // 上层最大含水量（只读）
    const double WLM; // 下层最大含水量（只读）
    const double WDM = std::numeric_limits<double>::infinity(); // 深层最大含水量（只读）
public:
    double WU = 0.0; // 上层当前含水量
    double WL = 70.0; // 下层当前含水量
    double WD = 80.0; // 深层当前含水量

    SoilState(double c, double wum, double wlm, double wdm = std::numeric_limits<double>::infinity(),
        double wu = 0.0, double wl = 70.0, double wd = 80.0);

    // 只读访问器，保证外部不能修改这些常量成员
    double getC()   const { return C; }
    double getWUM() const { return WUM; }
    double getWLM() const { return WLM; }
    double getWDM() const { return WDM; }
    double getW() const { return WU + WL + WD; }
};

// 各层蒸散发结果
struct EvapResult {
    double EU = 0.0;   // 上层蒸发量
    double EL = 0.0;   // 下层蒸发量
    double ED = 0.0;   // 深层蒸发量
	double E = 0.0;    // 总蒸散发量

	std::string stage = "未知阶段"; // 用于记录当前阶段的描述
};

// 计算一个时段的蒸散发
EvapResult evap_step(SoilState& s, const Environment& env);

#endif