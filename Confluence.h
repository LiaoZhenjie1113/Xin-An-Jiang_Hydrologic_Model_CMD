#ifndef CONFLUENCE_H
#define CONFLUENCE_H
#include <optional>
#include <limits>
#include <vector> //push_back
#include "HydroState.h"
#include "Evaporation.h"
#include "SourceDivision.h"

// 汇流参数输入
struct ConfluenceParamentInput {
    double CI = std::numeric_limits<double>::quiet_NaN(); // 壤中流消退系数
    double CG = std::numeric_limits<double>::quiet_NaN(); // 地下水消退系数
    double F = std::numeric_limits<double>::quiet_NaN(); // 流域总面积
    double DT = std::numeric_limits<double>::quiet_NaN(); // 计算时段，单位为小时，默认2小时
    std::vector<double> UH = {}; // 无因次单位线，默认4时段，为0.1、0.6、0.2、0.1
    //double L = std::numeric_limits<double>::quiet_NaN(); // 平移作用（滞时）
    std::optional<double> KE; // 马斯京根法演算参数，计算C0，C1，C2
    std::optional<double> XE; // 马斯京根法演算参数，计算C0，C1，C2
    std::optional<double> C0; // 本时段上游流量系数
	std::optional<double> C1; // 上时段上游流量系数
	std::optional<double> C2; // 上时段下游流量系数
};

// 汇流参数计算
class ConfluenceCal {
public:
    // 坡地汇流
    double CI; // 壤中流消退系数
    double CG; // 地下水消退系数
	// 河网汇流
    double DT; // 计算时段，单位为小时
    std::vector<double> UH; // 无因次单位线时段数
    //double L; // 平移作用（滞时）
    // 河道汇流
    double U; // 单位转换系数
	double C0; // 本时段上游流量系数
	double C1; // 上时段上游流量系数
	double C2; // 上时段下游流量系数

    ConfluenceCal(const ConfluenceParamentInput& input);

};

// 汇流结果
struct ConfluenceResult {
	std::vector<double> QS = {}; // 需要用户提供初始值，注意前面填充0.0
	std::vector<double> QI = {}; // 需要用户提供初始值，注意前面填充0.0
	std::vector<double> QG = {}; // 需要用户提供初始值，注意前面填充0.0
	std::vector<double> QT = {}; // 河网汇流结果，push_back QS.back() + QI.back() + QG.back() 到队尾，需要用户提供初始值
    std::vector<double> Q1 = {}; // 上游流量,需要用户提供初始值
	std::vector<double> Q2 = {}; // 下游流量，需要用户提供初始值
    double Q[2] = {std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::quiet_NaN()}; // 河道汇流结果，{上游Q1.back(),下游Q2.back()}
};

// 汇流计算
ConfluenceResult calConfluence(const Environment& env, const EvapResult& er, const ConfluenceCal& cal, ConfluenceResult& cres, SourceDivisionResult& sdr);

#endif