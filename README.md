# 新安江模型_CMD
## 项目简介
本项目实现了**新安江三水源流域水文模型**的 C++ 版本。模型基于蓄满产流理论，将径流划分为地表径流、壤中流和地下径流，并通过线性水库或马斯京根法进行汇流计算。程序以命令行为主要交互方式，读取降水、蒸发等输入数据（CSV 格式），输出流量过程等结果。
Copyright (C) 2026 廖振杰(Liao Zhenjie), Licensed under GNU GPL v2.0

## 开发环境
- 集成开发环境：Visual Studio 2022 (or later)
- 编译器：MSVC（VC++ 工具集）
- C++语言标准：ISO C++20标准（/std:c++20）
- 字符集：使用多字节字符集

## 文件结构
新安江模型_CMD/
├── main.cpp # 主程序入口
├── HydroState.cpp/.h # 水力状态更新模块
├── Evaporation.cpp/.h # 蒸发计算模块
├── Runoff.cpp/.h # 产流模块
├── SourceDivision.cpp/.h # 水源划分模块
├── Confluence.cpp/.h # 汇流模块
├── 新安江模型_CMD.sln # Visual Studio 解决方案
├── 新安江模型_CMD.vcxproj # 项目配置文件
├── Sample.csv # 示例输入数据
├── README.md # 本文件
└── LICENSE.txt # 许可证（GPL v2.0）

## 数据要求
示例数据Sample.csv，存放在执行文件的根目录
每一行为一条记录，不同属性用逗号分隔"，"
第一列：降雨量，单位mm；
第二列：蒸发皿蒸发量E0，单位mm；如果已经是EM，则在程序中将KC设置为1.0