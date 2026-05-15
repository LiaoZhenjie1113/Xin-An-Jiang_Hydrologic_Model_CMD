#ifndef SourceDivision_H
#define SourceDivision_H
#include <string>
#include <vector>
#include <optional>
#include <limits>
#include <stdexcept>
#include "HydroState.h"
#include "Evaporation.h"
#include "Runoff.h"

// 水源划分参数输入
struct SourceDivisionParaments {
	double EX = std::numeric_limits<double>::quiet_NaN(); // 抛物线指数
	double KI = std::numeric_limits<double>::quiet_NaN(); // 自由水蓄水库对壤中流的出流系数
	double KG = std::numeric_limits<double>::quiet_NaN(); // 自由水蓄水库对地下水的出流系数
	std::optional<double> SM; // 流域平均自由水蓄水容量
	std::optional<double> SMM; // 流域平均最大自由水蓄水容量
};


//水源划分参数处理
class SourceDivisionCal { //纵坐标为“自由水蓄水能力小于横坐标的面积FS/产流面积FR”，横坐标“为某点自由水容量SMF'”
private:
    double EX; // 抛物线指数
    double KI; // 自由水蓄水库对壤中流的出流系数
    double KG; // 自由水蓄水库对地下水的出流系数
    double SM; // 流域平均自由水蓄水容量
    double SMM; // 流域平均最大自由水蓄水容量

	// SMM, SM 互推
	static double calSMMFromSM(const double SM, const double EX);
	static double calSMFromSMM(const double SMM, const double EX);

public:
	SourceDivisionCal(const SourceDivisionParaments& sdp);

	double getEX() const { return EX; }
	double getKI() const { return KI; }
	double getKG() const { return KG; }
	double getSM() const { return SM; }
	double getSMM() const { return SMM; }
};

// 水源划分结果
struct SourceDivisionResult {
	std::vector<double> FR = {}; // 产流面积，需要用户提供初始值
	std::vector<double> S1 = {}; // 时段初自由水蓄量
	std::vector<double> SMMF = {}; // 流域上最大自由水蓄水容量 可与EX计算SMF
	std::vector<double> SMF = {}; // 产流面积上最大自由水蓄水容量（小曲线下面积） 可与EX计算SMMF
	std::vector<double> AU = {}; // 自由水深
	std::vector<double> S2 = {}; // 时段末自由水蓄量，需要用户提供初始值
	std::vector<double> Rs = {}; // 地面径流，水源划分计算
	double Ri = 0.0; // 壤中流 Ri = KI * S，水源划分计算
	double Rg = 0.0; // 地下径流 R = KG * S，水源划分计算
	double Rb = 0.0; // 不透水面积产流，水源划分IM计算（未实现）
	std::string DivisionDescription = "无水源划分";
};

// 水源划分计算
SourceDivisionResult SourceDivision(SourceDivisionCal& sdc, SourceDivisionResult& sdr, const Environment& env, const EvapResult& eres, const RunoffState& rs, const RunoffResult& rres);

#endif