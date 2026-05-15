// Copyright (C) 2026 廖振杰(Liao Zhenjie), Licensed under GNU GPL v2.0
#include "Evaporation.h"
#include <stdexcept>  //标准异常库
#include <algorithm>

//初始化和验证
SoilState::SoilState(double c, double wum, double wlm, double wdm, double wu, double wl, double wd)
    : C(c), WUM(wum), WLM(wlm), WDM(wdm), WU(wu), WL(wl), WD(wd) {
    if (c <= 0.0) {
        throw std::invalid_argument("参数 C 必须为正数！");
    }
    else if (c >= 0.5) {
        throw std::invalid_argument("参数 C 必须小于 0.5！");
    }
    if (wum <= 0.0) {
        throw std::invalid_argument("参数 WUM 必须为正数！");
    }
    else if (wu < 0.0 || wu > wum) {
        throw std::invalid_argument("参数 WU 必须在 0 和 WUM 之间！");
    }
    if (wlm <= 0.0) {
        throw std::invalid_argument("参数 WLM 必须为正数！");
    }
    else if (wl < 0.0 || wl > wlm) {
        throw std::invalid_argument("参数 WL 必须在 0 和 WLM 之间！");
    }
    if (wdm <= 0.0) {
        throw std::invalid_argument("参数 WDM 必须为正数！");
    }
    else if (wd < 0.0 || wd > wdm) {
        throw std::invalid_argument("参数 WD 必须在 0 和 WDM 之间！");
    }
}

// 计算一个时段的蒸散发，并更新土壤含水量
EvapResult evap_step(SoilState& s, const Environment& env) {

	// 创建结果结构体
    EvapResult eres;

    if (env.P >= env.EM) {
        eres.EU = env.EM; // 上层蒸发量为潜在蒸散发量
		eres.stage = "降水富裕";
	} else { // 降水量不大于潜在蒸散发量，进入蒸发阶段 P <= EM
        if (s.WU + env.P >= env.EM) {
			eres.EU = env.EM; // (1) 上层够蒸
			eres.stage = "上层够蒸";
        } else {
            eres.EU = s.WU + env.P;
            double remain = env.EM - eres.EU; // 还需要补足的蒸发量
            if (s.WL >= s.getC() * s.getWLM()) { // (2) 上层不够，下层充裕
                eres.EL = remain * s.WL / s.getWLM();
				eres.stage = "上层不够，下层充裕";
            } else if (s.WL >= s.getC() * remain && s.WL < s.getC() * s.getWLM()) { // (3) 上层不够，下层勉强
                eres.EL = s.getC() * remain;
				eres.stage = "上层不够，下层勉强";
            } else { // (4) 上层下层都不够
                eres.EL = s.WL;
                eres.ED = s.getC() * remain - eres.EL;
				eres.stage = "上层下层都不够";
            }
        }
	}
	eres.E = eres.EU + eres.EL + eres.ED; // 总蒸散发量
    return eres;
}
