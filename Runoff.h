#ifndef RUNOFF_H
#define RUNOFF_H
#include <string>
#include <limits>
#include <optional>
#include <vector> // 记录Rs历史,push_back
#include "HydroState.h"
#include "Evaporation.h"

//径流参数输入
struct RunoffParaments {
	double B = std::numeric_limits<double>::quiet_NaN(); // 流域张力水蓄水容量面积分配曲线指数 B
	double IM = std::numeric_limits<double>::quiet_NaN(); // 不透水面积
	std::optional<double> Wmm; // 流域最大点田间持水量 Wmm （纵坐标Wm的最大值）
	std::optional<double> WM; // 流域最大平均蓄水容量 WM （Wm-f/F曲线下方面积） = WUM + WLM + WDM
	std::optional<double> W0; // 初始平均蓄水量 W0（Wm=a下方∩Wm-f/F曲线下方面积）W0 = WU + WL + WD
	std::optional<double> A; // 初始平均W0时，最大点蓄水量
};

//径流参数设置
class RunoffState { //区分Wm（曲线）、Wmm（极大值）、WM（平均值）
private:
	double IM; // 不透水面积
	double B; // 流域张力水蓄水容量面积分配曲线指数 B
	double Wmm; // 流域最大点田间持水量 Wmm （纵坐标Wm的最大值）
	double WM; // 流域最大平均蓄水容量 WM （Wm-f/F曲线下方面积） = WUM + WLM + WDM
	//double fF = 0.0; // 所有田间持水量小于该Wm的面积f/流域总面积F（α）

		//方法1：输入 B、Wmm，自动计算 WM
	static double calWMFromWmm(const double B, const double Wmm);

	//方法2：输入 B、WM，自动计算 Wmm
	static double calWmmFromWM(const double B, const double WM);

	//方法3：输入 B、Wmm、WM、A，自动计算 W0
	static double calW0FromA(const double B, const double Wmm, const double WM, const double A);

	//方法4：输入 B、Wmm、WM、W0，计算 A
	static double calAFromW0(const double B, const double Wmm, const double WM, const double W0);

public:
	double W0 = 0.0; // 初始平均蓄水量 W0（Wm=a下方∩Wm-f/F曲线下方面积）W0 = WU + WL + WD
	double A = 0.0; // 初始平均W0时，最大点蓄水量

	RunoffState(const RunoffParaments& rp);

	double getB() const;
	double getIM() const;
	double getWmm() const;
	double getWM() const;
};

struct RunoffResult {
	double R = 0.0; // 径流量
	std::string Runoffdescription = "无径流产生";
};

RunoffResult runoff_step(const Environment& env, EvapResult& eres, RunoffState& rs, SoilState& ss);

#endif