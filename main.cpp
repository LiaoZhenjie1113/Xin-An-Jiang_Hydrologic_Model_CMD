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

#include <filesystem>
#include <iostream>
#include <fstream>
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

// 对齐三个 vector 的长度，较短的在前面填充0.0
void alignVectors(std::vector<double>& a, std::vector<double>& b, std::vector<double>& c) {
    size_t max_len = std::max({ a.size(), b.size(), c.size() });
    auto pad = [&](std::vector<double>& v) {
        if (v.size() < max_len) {
            v.insert(v.begin(), max_len - v.size(), 0.0);
        }
    };
    pad(a); pad(b); pad(c);
}

// 从CSV文件读取降雨蒸发数据
// 文件格式：第一行为标题行，例如 "P, E" 或 "P,E"
// 后续每行两个数值，用逗号分隔
std::vector<Environment> loadEnvFromCSV(const std::string& filename) {
    std::vector<Environment> envList;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + filename);
    }

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        // 跳过空行
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }
        // 跳过标题行（第一行）
        if (lineNum == 1) {
            continue;
        }

        // 解析逗号分隔的两个浮点数
        std::istringstream iss(line);
        std::string token;
        double P, E;
        // 读取第一个值（降水）
        if (!std::getline(iss, token, ',')) {
            std::cerr << "警告：第" << lineNum << "行格式错误，已跳过" << std::endl;
            continue;
        }
        try {
            P = std::stod(token);
        }
        catch (...) {
            std::cerr << "警告：第" << lineNum << "行降水值无效，已跳过" << std::endl;
            continue;
        }
        // 读取第二个值（蒸发）
        if (!std::getline(iss, token, ',')) {
            std::cerr << "警告：第" << lineNum << "行缺少蒸发值，已跳过" << std::endl;
            continue;
        }
        try {
            E = std::stod(token);
        }
        catch (...) {
            std::cerr << "警告：第" << lineNum << "行蒸发值无效，已跳过" << std::endl;
            continue;
        }

        envList.emplace_back(P, E);
    }

    if (envList.empty()) {
        throw std::runtime_error("文件中没有有效数据");
    }
    return envList;
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
    std::cout << "当前工作目录: " << std::filesystem::current_path() << std::endl;
    try {
        // ===================== 1. 初始化基础参数 =====================
        // 土壤参数（蒸发模块）
		double KC = getDoubleInput("请输入蒸发皿系数 Kc (若数据已是 EM 则输入 1.0)", 1.0); // 蒸发皿系数
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
        double F = getDoubleInput("请输入流域面积F (km2)", 537.0);     // 流域面积 (km²)
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
		//confluenceResult.QT = getVectorInput("请输入初始QT", { 60.0 }); QT = QS + QI + QG，自动计算，无需用户输入

		// 对齐QS、QI、QG三个vector的长度，较短的在前面填充0.0
        alignVectors(confluenceResult.QS, confluenceResult.QI, confluenceResult.QG);

		// 自动计算QT初始值
        auto calcQT = [](const std::vector<double>& qs,
                         const std::vector<double>& qi,
                         const std::vector<double>& qg) -> std::vector<double> {
            size_t n = std::max({ qs.size(), qi.size(), qg.size() });
            std::vector<double> qt(n, 0.0);
            for (size_t i = 0; i < n; ++i) {
                double qs_val = i < qs.size() ? qs[i] : 0.0;
                double qi_val = i < qi.size() ? qi[i] : 0.0;
                double qg_val = i < qg.size() ? qg[i] : 0.0;
                qt[i] = qs_val + qi_val + qg_val;
            }
            return qt;
        };
        confluenceResult.QT = calcQT(confluenceResult.QS, confluenceResult.QI, confluenceResult.QG);

        confluenceResult.Q1 = getVectorInput("请输入初始Q1", { 65.0 });
        confluenceResult.Q2 = getVectorInput("请输入初始Q2", { 65.0 });

        // ===================== 3. 模拟多时段水文过程 =====================
        // 模拟3个时段的降水和蒸发能力（可替换为实际观测数据）
        std::vector<Environment> envList;
        std::cout << "请输入降雨蒸发数据文件路径（CSV格式，第一行标题 P,E，直接回车使用内置示例数据）: ";
        std::string dataFile;
        std::getline(std::cin, dataFile);

        // 如果用户未指定文件，默认使用同目录下的 Sample.csv
        if (dataFile.empty()) {
            dataFile = "Sample.csv";
            KC = 0.68; // 示例数据的KC
            for (auto& env : envList) {
                env.EM *= KC;
            }
        }

        try {
            envList = loadEnvFromCSV(dataFile);
            // 将蒸发皿蒸发量 E0 转换为最大蒸发能力 EM
            for (auto& env : envList) {
                env.EM *= KC;
            }
            std::cout << "成功从文件 " << dataFile << " 加载 " << envList.size() << " 个时段的数据。" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "加载数据文件失败: " << e.what() << std::endl;
            std::cerr << "程序退出。" << std::endl;
            return 1;
        }

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