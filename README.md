# MFCApplication2 — 迷宫 AGV 路径规划与仿真系统

基于 MFC 框架的迷宫自动引导车辆（AGV）路径规划与仿真程序，是 C++ 课程大作业。

## 功能特性

- **随机迷宫生成**：基于随机深度优先搜索（Randomized DFS）算法
- **最短路径规划**：Dijkstra 算法
- **最快路径规划**：在 Dijkstra 基础上引入转向时间成本，减少 AGV 转弯次数
- **AGV 仿真动画**：定时器驱动的自动导航演示
- **参数可调**：支持自定义迷宫行/列数、移动速度、转向时间

## 技术栈

- C++（STL：`std::vector` / `std::priority_queue` / `std::queue`）
- MFC 对话框框架 + GDI 绘图
- 算法：Randomized DFS、Dijkstra、带方向权重的路径优化、Union-Find

## 编译

- 环境：Visual Studio（v145 工具集），Windows 10
- 平台：x64
- 配置：Debug

```powershell
MSBuild MFCApplication2.vcxproj /p:Configuration=Debug /p:Platform=x64
```

## 使用

1. 设置「行数 / 列数」（5–50）后点击「生成迷宫」
2. 点击「开始」启动 AGV 沿路径移动
3. 点击「停止」暂停动画

## 目录结构

```
MFCApplication2.cpp / .h       应用程序入口
MFCApplication2Dlg.cpp / .h    主对话框（迷宫生成、路径规划、绘制、AGV 控制）
MFCApplication2.rc             资源文件
resource.h                     资源 ID 定义
framework.h / pch.h / pch.cpp  MFC 框架与预编译头
targetver.h                    目标平台版本
res/                           图标等资源
```
