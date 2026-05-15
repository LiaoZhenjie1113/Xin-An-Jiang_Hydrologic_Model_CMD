// Copyright (C) 2026 廖振杰(Liao Zhenjie), Licensed under GNU GPL v2.0
#include <stdexcept>
#include "SourceDivision.h"

SourceDivisionCal::SourceDivisionCal(const SourceDivisionParaments& sdp) {
	//检查必填参数
	if (std::_Is_nan(sdp.EX)) {
		throw std::invalid_argument("缺少必填参数 EX");
	} else {
		this->EX = sdp.EX;
	}
	if (std::_Is_nan(sdp.KI)) {
		throw std::invalid_argument("缺少必填参数 KI");
	} else {
		this->KI = sdp.KI;
	}
	if (std::_Is_nan(sdp.KG)) {
		throw std::invalid_argument("缺少必填参数 KG");
	} else {
		this->KG = sdp.KG;
	}
	if (sdp.KI + sdp.KG >= 1.0) {
		throw std::invalid_argument("参数 KI 和 KG 的和必须小于 1！");
	}


	//处理 SMM, SM
	if(!sdp.SMM.has_value() && sdp.SM.has_value()) {
		this->SMM = calSMMFromSM(sdp.SM.value(), sdp.EX);
	} else if (sdp.SMM.has_value() && !sdp.SM.has_value()) {
		this->SM = calSMFromSMM(sdp.SMM.value(), sdp.EX);
	} else if (sdp.SMM.has_value() && sdp.SM.has_value()) {
		double calculated_SMM = calSMMFromSM(sdp.SM.value(), sdp.EX);
		if (std::abs(calculated_SMM - sdp.SMM.value()) > 1e-6) {
			throw std::invalid_argument("提供的 SMM 和 SM 不一致！");
		}
		this->SMM = sdp.SMM.value();
		this->SM = sdp.SM.value();
	} else {
		throw std::invalid_argument("无法确定 SMM 和 SM，请至少提供其中一个");
	}

	//保存结果
	this->EX = sdp.EX;
	this->KI = sdp.KI;
	this->KG = sdp.KG;
}

double SourceDivisionCal::calSMMFromSM(const double SM, const double EX) {
	double SMM = SM *(1.0 + EX);
	return SMM;
}

double SourceDivisionCal::calSMFromSMM(const double SMM, const double EX) {
	double SM = SMM / (1.0 + EX);
	return SM;
}


SourceDivisionResult SourceDivision(SourceDivisionCal& sdc, SourceDivisionResult& sdr, const Environment& env, const EvapResult& eres, const RunoffState& rs, const RunoffResult& rres) {
	// 验证初始值
	if (sdr.FR.empty()) {
		throw std::invalid_argument("初始值 FR 必须提供");
	}
	if (sdr.S2.empty()) {
		throw std::invalid_argument("初始值 S2 必须提供");
	}

	// 水源划分
	if (env.P <= eres.E) {
		sdr.FR.push_back(1.0 - pow(1.0 - rs.W0 / rs.getWM(), rs.getB() / (1.0 + rs.getB())));
		sdr.S1.push_back(sdr.S2.back() * sdr.FR[sdr.FR.size() - 2] / sdr.FR.back());
		sdr.SMMF.push_back(sdc.getSMM() * (1.0 - pow(1.0 - sdr.FR.back(), 1.0 / sdc.getEX())));
		sdr.SMF.push_back(sdr.SMMF.back() / (1.0 + sdc.getEX()));
		sdr.AU.push_back(sdr.SMMF.back() * (1.0 - pow(1.0 - sdr.S1.back() / sdr.SMF.back(), 1.0 / (1.0 + sdc.getEX()))));
		sdr.Rs.push_back(0.0);
		sdr.Ri = sdr.S1.back() * sdc.getKI() * sdr.FR.back();
		sdr.Rg = sdr.S1.back() * sdc.getKG() * sdr.FR.back();
		sdr.S2.push_back(sdr.S1.back() * (1.0 - sdc.getKI() - sdc.getKG()));
		sdr.DivisionDescription = "无有效净雨，无地面径流";
	} else {
		double FR_origion = rres.R / (env.P - eres.E);
		if (FR_origion > 1.0) {
			FR_origion = 1.0; //！！！注意浮点数有精度误差，导致SMMF计算错误为nan
		}
		sdr.FR.push_back(FR_origion);
		sdr.S1.push_back(sdr.S2.back() * sdr.FR[sdr.FR.size() - 2] / sdr.FR.back());
		sdr.SMMF.push_back(sdc.getSMM() * (1.0 - pow(1.0 - sdr.FR.back(), 1.0 / sdc.getEX())));
		sdr.SMF.push_back(sdr.SMMF.back() / (1.0 + sdc.getEX()));
		sdr.AU.push_back(sdr.SMMF.back() * (1.0 - pow(1.0 - sdr.S1.back() / sdr.SMF.back(), 1.0 / (1.0 + sdc.getEX()))));
		if (env.P - eres.E + sdr.AU.back() >= sdr.SMMF.back()) {
			sdr.Rs.push_back(sdr.FR.back() * (env.P - eres.E + sdr.S1.back() - sdr.SMF.back()));
			sdr.Ri = sdr.SMF.back() * sdc.getKI() * sdr.FR.back();
			sdr.Rg = sdr.SMF.back() * sdc.getKG() * sdr.FR.back();
			sdr.S2.push_back(sdr.SMF.back() - (sdr.Ri + sdr.Rg) / sdr.FR.back());
			sdr.DivisionDescription = "自由水库蓄满，全流域产流";
		}
		else if (0.0 < env.P - eres.E + sdr.AU.back() && env.P - eres.E + sdr.AU.back() < sdr.SMMF.back()) {
			sdr.Rs.push_back(sdr.FR.back() * (env.P - eres.E + sdr.S1.back() - sdr.SMF.back() + sdr.SMF.back() * pow(1.0 - (env.P - eres.E + sdr.AU.back()) / sdr.SMMF.back(), 1.0 + sdc.getEX())));
			sdr.Ri = sdc.getKI() * sdr.FR.back() * (env.P - eres.E + sdr.S1.back() - sdr.Rs.back() / sdr.FR.back());
			sdr.Rg = sdc.getKG() * sdr.FR.back() * (env.P - eres.E + sdr.S1.back() - sdr.Rs.back() / sdr.FR.back());
			sdr.S2.push_back(sdr.S1.back() + env.P - eres.E - (sdr.Rs.back() + sdr.Ri + sdr.Rg) / sdr.FR.back());
			sdr.DivisionDescription = "产流面积内自由水库未蓄满，部分产流，";
		}
	}
	return sdr;
}