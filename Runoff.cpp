// Copyright (C) 2026 廖振杰(Liao Zhenjie), Licensed under GNU GPL v2.0
#include <stdexcept>  //标准异常库
#include <windows.h> //该头文件包括了min()的宏定义
#include "Runoff.h"


//类通用构造函数
RunoffState::RunoffState(const RunoffParaments& rp)
    : B(rp.B), IM(rp.IM) {
    if (std::_Is_nan(rp.B)) {
        throw std::invalid_argument("参数 B 必须是有效的数字！");
    } else if (rp.B <= 0.0 || rp.B >= 0.6) {
        throw std::invalid_argument("参数 B 必须在 0 和 0.6 之间！");
    }
    if (std::_Is_nan(rp.IM) || rp.IM < 0.0) {
		this->IM = 0.0; // 默认不透水面积为0
        MessageBox(NULL, "无有效IM输入，默认设置为0", "提示", MB_OK | MB_ICONASTERISK);
    } else if (rp.IM > 1.0) {
        this->IM = 1.0; // 默认不透水面积为1
        MessageBox(NULL, "无有效IM输入，默认设置为1", "提示", MB_OK | MB_ICONASTERISK);
	}

    // 处理 Wmm 和 WM 的关系
    if (rp.Wmm.has_value() && !rp.WM.has_value()) {
        this->Wmm = rp.Wmm.value();
        this->WM = calWMFromWmm(this->B, this->Wmm);
    } else if (!rp.Wmm.has_value() && rp.WM.has_value()) {
        this->WM = rp.WM.value();
        this->Wmm = calWmmFromWM(this->B, this->WM);
    } else if (rp.Wmm.has_value() && rp.WM.has_value()) {
        this->Wmm = rp.Wmm.value();
        this->WM = rp.WM.value();
        double calculated_WM = calWMFromWmm(this->B, this->Wmm);
        if (std::abs(calculated_WM - this->WM) > 1e-6) {
            throw std::invalid_argument("提供的 Wmm 和 WM 不一致！");
        }
    } else {
        throw std::invalid_argument("请至少提供 Wmm 或 WM 中的一个参数！");
    }

     // 处理 W0 和 A 的关系
    if (rp.W0.has_value() && !rp.A.has_value()) {
        this->W0 = rp.W0.value();
        this->A = calAFromW0(this->B, this->Wmm, this->WM, this->W0);
    } else if (!rp.W0.has_value() && rp.A.has_value()) {
        this->A = rp.A.value();
        this->W0 = calW0FromA(this->B, this->Wmm, this->WM, this->A);
    } else if (rp.W0.has_value() && rp.A.has_value()) {
        this->W0 = rp.W0.value();
        this->A = rp.A.value();
        double calculated_W0 = calW0FromA(this->B, this->Wmm, this->WM, this->A);
        if (std::abs(calculated_W0 - this->W0) > 1e-6) {
            throw std::invalid_argument("提供的 W0 和 A 不一致！");
        }
    } else {
        throw std::invalid_argument("请至少提供 W0 或 A 中的一个参数！");
	}
}

//方法1：输入 B、Wmm，自动计算 WM
double RunoffState::calWMFromWmm(const double B, const double Wmm) {
    double WM = Wmm / (B + 1.0); // 根据 Wmm 和 B 计算 WM
    return WM;
}

//方法2：输入 B、WM，自动计算 Wmm
double RunoffState::calWmmFromWM(const double B, const double WM) {
    double Wmm = WM * (B + 1.0); // 根据 WM 计算 Wmm
    return Wmm;
}

//方法3：输入 B、Wmm、WM、A，自动计算 W0
double RunoffState::calW0FromA(const double B, const double Wmm, const double WM, const double A) {
    double W0 = Wmm / (B + 1.0) * (1 - pow(1.0 - A / Wmm, 1.0 + B)); // 根据 A 计算 W0
    return W0;
}

//方法4：输入 B、WM、W0，自动计算 A
double RunoffState::calAFromW0(const double B, const double Wmm, const double WM, const double W0) {
    double A = Wmm * (1 - pow(1.0 - W0 / WM, 1.0 / (1.0 + B))); // 根据 W0 计算 A
    return A;
}

double RunoffState::getB() const { return B; }

double RunoffState::getIM() const { return IM; }

double RunoffState::getWmm() const { return Wmm; }

double RunoffState::getWM() const { return WM; }

RunoffResult runoff_step(const Environment& env, EvapResult& eres, RunoffState& rs, SoilState& ss) {
	RunoffResult rres;
	if (env.P <= eres.E) {
		rres.R = 0.0; // 无径流产生
		rres.Runoffdescription = "无径流产生";
	} else if (env.P - eres.E + rs.A >= rs.getWmm()) {
		rres.R = env.P - eres.E - rs.getWM() + rs.W0; // 全流域产流
		rres.Runoffdescription = "全流域产流";
	} else {
		rres.R = env.P - eres.E - rs.getWM() + rs.W0 + rs.getWM() * pow(1.0 - (env.P - eres.E + rs.A) / rs.getWmm(), 1.0 + rs.getB()); //局部产流
		rres.Runoffdescription = "局部产流";
	}
	rs.W0 = min(rs.W0 + env.P - eres.E - rres.R, rs.getWmm()); // 更新平均蓄水量 W0
	rs.A = rs.getWmm() * (1 - pow(1.0 - rs.W0 * (rs.getB() + 1.0) / rs.getWmm(), 1.0 / (1.0 + rs.getB()))); // 更新最大点蓄水量 A

    // 结果验证
    if (rres.R < 0) {
        throw std::runtime_error("计算错误：产流量 R 不应为负数！");
	}

	// 更新土壤状态
    if (ss.WU + env.P - eres.E - rres.R <= ss.getWUM()) { //更新各层土壤含水量
        ss.WU = min(ss.WU + env.P - eres.E - rres.R, ss.getWUM()); // 上层含水量增加
        eres.stage = "上层含水量增加";
    }
    else if (ss.WU + ss.WL + env.P - eres.E - rres.R - ss.getWUM() <= ss.getWLM()) {
        ss.WL = min(ss.WL + ss.WU + env.P - eres.E - rres.R - ss.getWUM(), ss.getWLM()); // 下层含水量增加
        ss.WU = ss.getWUM(); // 上层含水量达到最大值
        eres.stage = "下层含水量增加";
    }
    else {
        ss.WD = min(ss.WD + ss.WU + ss.WL + env.P - eres.E - rres.R - ss.getWUM() - ss.getWLM(), ss.getWDM()); // 深层含水量增加
        ss.WU = ss.getWUM(); // 上层含水量达到最大值
        ss.WL = ss.getWLM(); // 下层含水量达到最大值
        eres.stage = "深层含水量增加";
    }

    // 防止数值误差出现负数
    if (ss.WU < 0) ss.WU = 0;
    if (ss.WL < 0) ss.WL = 0;
    if (ss.WD < 0) ss.WD = 0;

    // 返回产流结果
	return rres;
}