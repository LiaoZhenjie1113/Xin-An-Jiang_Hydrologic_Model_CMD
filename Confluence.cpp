// Copyright (C) 2026 廖振杰(Liao Zhenjie), Licensed under GNU GPL v2.0
#include <stdexcept>
#include <windows.h>
#include "Confluence.h"

ConfluenceCal::ConfluenceCal(const ConfluenceParamentInput& input) {
	// 验证输入参数
    if (std::_Is_nan(input.CI)) {
        throw std::invalid_argument("缺少必填参数 CI");
    } else if (input.CI < 0.0 || input.CI > 1.0) {
        throw std::invalid_argument("参数 CI 必须在 0 和 1 之间！");
    }
    if (std::_Is_nan(input.CG)) {
        throw std::invalid_argument("缺少必填参数 CG");
    } else if (input.CG < 0.0 || input.CG > 1.0) {
        throw std::invalid_argument("参数 CG 必须在 0 和 1 之间！");
	}
    if (std::_Is_nan(input.F)) {
        throw std::invalid_argument("缺少必填参数 F");
	} else if (input.F <= 0.0) {
        throw std::invalid_argument("参数 F 必须大于 0！");
	}
    if (std::_Is_nan(input.DT)|| input.DT <= 0) {
		this->DT = 2.0; // 默认2小时
        MessageBox(NULL, "缺少有效参数 DT，默认设置为2小时", "提示", MB_OK | MB_ICONASTERISK);
    } else {
		this->DT = input.DT;
    }
    if (input.UH.size() < 2) {
		this->UH = { 0.1, 0.6, 0.2, 0.1 }; // 默认4时段单位线
        MessageBox(NULL, "无有效时段单位线，默认设置为0.1, 0.6, 0.2, 0.1", "提示", MB_OK | MB_ICONASTERISK);
    } else {
		this->UH = input.UH;
    }
    /*
    if (std::_Is_nan(input.L)) {
        throw std::invalid_argument("缺少必填参数 L");
    } else if (input.L < 0.0) {
        throw std::invalid_argument("参数 L 必须大于等于 0！");
    }
    */
	this->CI = input.CI;
	this->CG = input.CG;
	//this->L = input.L;
	this->U = input.F / (3.6 * input.DT);
    
	// 计算C0、C1、C2
    if (!input.C0.has_value() || !input.C1.has_value() || !input.C2.has_value()) {
        if (!input.KE.has_value() || !input.XE.has_value()) {
            throw std::invalid_argument("缺少计算 C0、C1、C2 的参数 KE 或 XE");
        }
		this->C0 = (0.5 * input.DT - input.KE.value() * input.XE.value()) / (input.KE.value() - input.KE.value() * input.XE.value() + 0.5 * input.DT);
        this->C1 = (0.5 * input.DT + input.KE.value() * input.XE.value()) / (input.KE.value() - input.KE.value() * input.XE.value() + 0.5 * input.DT);
        this->C2 = (input.KE.value() - input.KE.value() * input.XE.value() - 0.5 * input.DT) / (input.KE.value() - input.KE.value() * input.XE.value() + 0.5 * input.DT);
    } else {
        this->C0 = input.C0.value();
        this->C1 = input.C1.value();
        this->C2 = input.C2.value();
    }
}
// 汇流计算，需要用户提供Q1和Q2的第一个值
ConfluenceResult calConfluence(const Environment& env, const EvapResult& er, const ConfluenceCal& cal, ConfluenceResult& cres, SourceDivisionResult& sdr) {
    // 验证初始值
    if (sdr.Rs.size() < cal.UH.size()) {
        size_t zeros_to_add = cal.UH.size() - sdr.Rs.size();
        sdr.Rs.insert(sdr.Rs.begin(), zeros_to_add, 0.0);
	}
    if (cres.QS.empty()) {
        throw std::invalid_argument("初始值 QS 必须提供");
	}
    if (cres.QI.empty()) {
        throw std::invalid_argument("初始值 QI 必须提供");
    }
    if (cres.QG.empty()) {
        throw std::invalid_argument("初始值 QG 必须提供");
	}
    if (cres.QT.size() < cal.UH.size() - 1) {
        size_t zeros_to_add = cal.UH.size() - 1 -cres.QT.size();
        cres.QT.insert(cres.QT.begin(), zeros_to_add, 0.0);
    }
    if (cres.Q1.empty()) {
        throw std::invalid_argument("初始值 Q1 必须提供");
	}
    if (cres.Q2.empty()) {
        throw std::invalid_argument("初始值 Q2 必须提供");
    }
    // 计算坡地汇流
    double QSaccumulated = 0.0;
    for (size_t i = 0; i < cal.UH.size(); i++) {
		QSaccumulated += cal.UH[i] * sdr.Rs[sdr.Rs.size() - i - 1];
    }
    cres.QS.push_back(cal.U * QSaccumulated);
	cres.QI.push_back(cal.CI * cres.QI.back() + (1.0 - cal.CI) * sdr.Ri * cal.U);
	cres.QG.push_back(cal.CG * cres.QG.back() + (1.0 - cal.CG) * sdr.Rg * cal.U);

	// 计算河网汇流
	cres.QT.push_back(cres.QS.back() + cres.QI.back() + cres.QG.back());
	double Q1accumulated = 0.0;
    for (size_t i = 0; i < cal.UH.size(); i++) {
        Q1accumulated += cal.UH[i] * cres.QT[cres.QT.size() - i - 1];
    }
    cres.Q1.push_back(Q1accumulated);

	// 计算河道汇流
	cres.Q2.push_back(cal.C0 * cres.Q1.back() + cal.C1 * cres.Q1[cres.Q1.size() - 2] + cal.C2 * cres.Q2.back());
	cres.Q[0] = cres.Q1.back();
	cres.Q[1] = cres.Q2.back();

    return cres;
}
