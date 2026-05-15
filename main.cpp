/*
新安江模型_CMD（Xin_An_Jiang_Hydrological_Model_Command_Line_Interface）
用于模拟流域水文过程，包括蒸发、产流、水源划分和汇流等模块。
该程序基于C++编写，提供了一个命令行界面，允许用户输入环境参数并查看每个时段的计算结果。
Copyright(C) 2026 廖振杰(Liao Zhenjie)

This program is free software; you can redistribute it and /or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
*/

#include <iostream>
#include <sstream>
#include <vector>
#include "HydroState.h"
#include "Evaporation.h"
#include "Runoff.h"
#include "SourceDivision.h"
#include "Confluence.h"

// 读取参数，支持默认值
double getDoubleInput(const std::string& prompt, double defaultValue) {
    std::cout << prompt << " (默认: " << defaultValue << "): ";
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
        return defaultValue;
    }
    try {
        return std::stod(input);
    }
    catch (const std::invalid_argument&) {
        std::cerr << "输入无效，使用默认值: " << defaultValue << std::endl;
        return defaultValue;
    }
}

// 读取初始值，按空格分隔解析为 vector<double>，支持默认值
std::vector<double> getVectorInput(const std::string& prompt, const std::vector<double>& defaultValues) {
    std::cout << prompt << " (默认: ";
    for (size_t i = 0; i < defaultValues.size(); ++i) {
        if (i != 0) std::cout << " ";
		std::cout << defaultValues[i];
    }
    std::cout << ")" << "以空格分隔（直接回车则使用默认值）：";
    
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
        return defaultValues;
    }
    
    std::istringstream iss(input);
    std::vector<double> result;
    double val;
    while (iss >> val) {
        result.push_back(val);
    }

    // 如果非空行却没有解析出任何有效数字，视为输入错误，可回退默认值或提示重试
    if (result.empty()) {
        std::cout << "输入无效，未找到有效数字，使用默认值。" << std::endl;
        return defaultValues;
    }
    return result;
}

// 打印蒸发结果
void printEvapResult(const EvapResult& er, int step) {
    std::cout << "==== 第" << step << "时段蒸发计算结果 ====" << std::endl;
    std::cout << "总蒸发量 E: " << er.E << std::endl;
    std::cout << "上层蒸发 EU: " << er.EU << std::endl;
    std::cout << "下层蒸发 EL: " << er.EL << std::endl;
    std::cout << "深层蒸发 ED: " << er.ED << std::endl;
    std::cout << "蒸发阶段: " << er.stage << std::endl;
    std::cout << "==========================" << std::endl;
}

// 打印产流结果
void printRunoffResult(const RunoffResult& rr, int step) {
    std::cout << "==== 第" << step << "时段产流计算结果 ====" << std::endl;
    std::cout << "径流量 R: " << rr.R << std::endl;
    std::cout << "产流描述: " << rr.Runoffdescription << std::endl;
    std::cout << "==========================" << std::endl;
}

// 打印水源划分结果
void printSourceDivisionResult(const SourceDivisionResult& sdr, int step) {
    std::cout << "==== 第" << step << "时段水源划分结果 ====" << std::endl;
    std::cout << "地表径流 Rs: " << (sdr.Rs.empty() ? 0 : sdr.Rs.back()) << std::endl;
    std::cout << "壤中流 Ri: " << sdr.Ri << std::endl;
    std::cout << "地下径流 Rg: " << sdr.Rg << std::endl;
    std::cout << "划分描述: " << sdr.DivisionDescription << std::endl;
    std::cout << "==========================" << std::endl;
}

// 打印汇流结果
void printConfluenceResult(const ConfluenceResult& cr, int step) {
    std::cout << "==== 第" << step << "时段汇流计算结果 ====" << std::endl;
    std::cout << "地表径流汇流 QS: " << (cr.QS.empty() ? 0 : cr.QS.back()) << std::endl;
    std::cout << "壤中流汇流 QI: " << (cr.QI.empty() ? 0 : cr.QI.back()) << std::endl;
    std::cout << "地下径流汇流 QG: " << (cr.QG.empty() ? 0 : cr.QG.back()) << std::endl;
    std::cout << "总入流 QT: " << (cr.QT.empty() ? 0 : cr.QT.back()) << std::endl;
    std::cout << "上游流量 Q1: " << (cr.Q1.empty() ? 0 : cr.Q1.back()) << std::endl;
    std::cout << "下游流量 Q2: " << (cr.Q2.empty() ? 0 : cr.Q2.back()) << std::endl;
    std::cout << "流量情况 [上游, 下游]: [" << cr.Q[0] << ", " << cr.Q[1] << "]" << std::endl;
    std::cout << "==========================" << std::endl;
}

int main() {
    try {
        // ===================== 1. 初始化基础参数 =====================
        // 土壤参数（蒸发模块）
        double C = getDoubleInput("请输入深层蒸发系数C", 0.1);       // 深层蒸发系数
        double WUM = getDoubleInput("请输入上层土壤最大含水量WUM", 20.0);    // 上层土壤最大含水量
        double WLM = getDoubleInput("请输入下层土壤最大含水量WLM", 75.0);    // 下层土壤最大含水量
        double WDM = getDoubleInput("请输入深层土壤最大含水量WDM", 80.0);   // 深层土壤最大含水量
        double initWU = getDoubleInput("请输入上层初始含水量initWU", 0.0); // 上层初始含水量
        double initWL = getDoubleInput("请输入下层初始含水量initWL", 70.0); // 下层初始含水量
        double initWD = getDoubleInput("请输入深层初始含水量initWD", 80.0);// 深层初始含水量

        // 产流参数
        double B = getDoubleInput("请输入蓄水容量曲线指数B", 0.3);       // 蓄水容量曲线指数
        double IM = getDoubleInput("请输入不透水面积比例IM", 0.0);     // 不透水面积比例
        double WM = WUM + WLM + WDM;   // 最大蓄水容量
        double W0 = initWU + initWL + initWD;     // 初始平均含水量

        // 水源划分参数
        double EX = getDoubleInput("请输入水源划分指数EX", 1.5);      // 水源划分指数
        double KI = getDoubleInput("请输入壤中流系数KI", 0.4);      // 壤中流系数
        double KG = getDoubleInput("请输入地下径流系数KG", 0.3);     // 地下径流系数
        double SM = getDoubleInput("请输入平均自由水蓄水容量SM", 20.0);     // 平均自由水蓄水容量

        // 汇流参数
        double CI = getDoubleInput("请输入壤中流消退系数CI", 0.6);      // 壤中流消退系数
        double CG = getDoubleInput("请输入地下径流消退系数CG", 0.98);      // 地下径流消退系数
        double F = getDoubleInput("请输入流域面积F (km²)", 537.0);     // 流域面积 (km²)
        double DT = getDoubleInput("请输入计算时段DT (小时)", 2.0);      // 计算时段 (小时)
        std::vector<double> UH = getVectorInput("请输入单位线UH", { 0.1, 0.6, 0.2, 0.1 }); // 单位线
        double KE = getDoubleInput("请输入马斯京根参数KE", 2.0);      // 马斯京根参数
        double XE = getDoubleInput("请输入马斯京根参数XE", 0.4);      // 马斯京根参数
        // ===================== 2. 初始化模块实例 =====================
        // 土壤状态初始化
        SoilState soilState(C, WUM, WLM, WDM, initWU, initWL, initWD);

        // 产流参数与状态初始化
        RunoffParaments runoffParams;
        runoffParams.B = B;
        runoffParams.IM = IM;
        runoffParams.WM = WM;
        runoffParams.W0 = W0;
        RunoffState runoffState(runoffParams);

        // 水源划分参数与状态初始化
        SourceDivisionParaments sdParams;
        sdParams.EX = EX;
        sdParams.KI = KI;
        sdParams.KG = KG;
        sdParams.SM = SM;
        SourceDivisionCal sdCal(sdParams);
        SourceDivisionResult sdResult;
        // 水源划分初始值
        sdResult.FR = { 0.1 };    // 初始径流系数
        sdResult.S2 = { 20.0 };   // 初始自由水蓄水量

        // 汇流参数与状态初始化
        ConfluenceParamentInput confluenceInput;
        confluenceInput.CI = CI;
        confluenceInput.CG = CG;
        confluenceInput.F = F;
        confluenceInput.DT = DT;
        confluenceInput.UH = UH;
        confluenceInput.KE = KE;
        confluenceInput.XE = XE;
        ConfluenceCal confluenceCal(confluenceInput);
        ConfluenceResult confluenceResult;
        // 汇流初始值
        confluenceResult.QS = getVectorInput("请输入初始QS", { 0.0 });
        confluenceResult.QI = getVectorInput("请输入初始QI", { 40.0 });
        confluenceResult.QG = getVectorInput("请输入初始QG", { 20.0 });
        confluenceResult.QT = getVectorInput("请输入初始QT", { 60.0 });
        confluenceResult.Q1 = getVectorInput("请输入初始Q1", { 65.0 });
        confluenceResult.Q2 = getVectorInput("请输入初始Q2", { 65.0 });

        // ===================== 3. 模拟多时段水文过程 =====================
        // 模拟3个时段的降水和蒸发能力（可替换为实际观测数据）
        std::vector<Environment> envList = {
            Environment(10.0, 0.068),   // 时段1: 降水10mm，潜在蒸发0.1mm
            Environment(24.1, 0.0),
            Environment(20.4, 0.068),
            Environment(18.3, 0.340),
            Environment(10.1, 0.476),
            Environment(5.5, 0.612),
			Environment(0.6, 0.544),
            Environment(3.1, 0.476),
            Environment(1.9, 0.34),
			Environment(4.6, 0.204),
            Environment(5.0, 0.136),
			Environment(4.8, 0.068),
			Environment(36.2, 0.0),
            Environment(29.0, 0.0),
			Environment(6.0, 0.068),
            Environment(3.6, 0.408),
            Environment(0.4, 0.544),
            Environment(0.0, 0.680),
            Environment(0.5, 0.612),
            Environment(3.8, 0.544),
			Environment(0.0, 0.476),
            Environment(1.8, 0.340),
			Environment(0.2, 0.204),
            Environment(0.3, 0.068)
        };

        // 逐时段计算
        for (int i = 0; i < envList.size(); ++i) {
            int step = i + 1;
            Environment& env = envList[i];

            // 步骤1: 蒸发计算
            EvapResult evapResult = evap_step(soilState, env);
            printEvapResult(evapResult, step);

            // 步骤2: 产流计算
            RunoffResult runoffResult = runoff_step(env, evapResult, runoffState, soilState);
            printRunoffResult(runoffResult, step);

            // 步骤3: 水源划分
            sdResult = SourceDivision(sdCal, sdResult, env, evapResult, runoffState, runoffResult);
            printSourceDivisionResult(sdResult, step);

            // 步骤4: 汇流计算
            confluenceResult = calConfluence(env, evapResult, confluenceCal, confluenceResult, sdResult);
            printConfluenceResult(confluenceResult, step);

            std::cout << "\n----------------------------------------\n" << std::endl;
        }

    }
    
    catch (const std::invalid_argument& e) {
        std::cerr << "参数错误: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "运行时错误: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "未知错误发生" << std::endl;
        return 1;
    }
    
    return 0;
}