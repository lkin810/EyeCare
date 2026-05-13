# EyeCare v6.2

轻量级 Windows 护眼助手 — 蓝光过滤 & 定时休息，像素纸风格 UI

## 截图

<img width="1080" height="900" alt="image" src="https://github.com/user-attachments/assets/02c5f785-48e4-4fff-b275-1638c18368b8" />


## 功能

### 蓝光过滤
- **温和模式**：减少蓝光较多，适合夜间使用
- **办公模式**：轻度滤光，保持色彩准确
- 亮度滑块实时调节，即改即生效

### 定时休息
- **自定义策略**：自由设置工作时长和休息时长
- **20-20-20 预设**：每用眼 20 分钟，休息 20 秒，远眺 20 英尺
- 全屏休息屏：像素翻钟倒计时 + 进度条 + 8 条护眼动作指引
- 休息结束气泡通知

### 用眼统计
- 今日休息完成率（完成/跳过/总计）
- 近 7 天统计面板：用眼时长、休息完成率趋势柱状图
- 数据存储在 `%APPDATA%\EyeCare\stats.dat`，自动保留 365 天

### 空闲检测
- 离开电脑超过 3 分钟自动暂停计时
- 回来后继续倒计时，不会"虚计"用眼时间
- 主面板实时显示"空闲中"状态

### 连续用眼警告
- 连续用眼超过 90 分钟：琥珀色警告 + 托盘气泡提醒
- 连续用眼超过 2 小时：红色警告 + 托盘气泡每 10 分钟重复提醒
- 完成休息后重置计数

### 开机自启
- 一键开关，写入注册表，无需管理员权限

### 系统托盘
- 最小化到托盘，右键菜单快速操作
- 悬停显示当前状态

## UI 特色

像素纸风格（Pixel Paper Style）定制 UI：
- 16px 网格底纹 + 纸质横线纹理
- 2px 硬描边 + 分级硬偏移阴影（2px / 3px / 4px）
- 1px Bevel 浮雕高光/暗边
- 方形像素控件（开关、滑块、按钮）
- 翻钟式像素点阵倒计时
- 中英文字体自动切换（微软雅黑 / Consolas）
- DPI 感知，HiDPI 自适应
- GDI+ 双缓冲无闪烁绘制

## 编译

### 方法一：MSVC（推荐，误报率最低）

1. 打开 **x64 Native Tools Command Prompt for VS 2022**
2. `cd` 到项目目录
3. 运行 `build.bat`

输出：`EyeCare.exe`

### 方法二：Linux 交叉编译

1. 安装 llvm-mingw 或 mingw-w64 工具链
2. 运行 `bash build.sh`

> 注意：MinGW 编译的 exe 可能被杀软误报，建议用 MSVC 编译发布。

## 项目结构

```
EyeCare/
├── EyeCare.cpp          # 主源码（单文件架构，~2500 行）
├── resource.h           # 资源 ID 定义
├── EyeCare.rc           # 资源脚本（图标 + 版本信息 + 清单嵌入）
├── EyeCare.manifest     # 应用清单（UAC/DPI/兼容性声明）
├── EyeCare.ico          # 应用图标（多尺寸：16~256px）
├── build.bat            # MSVC 编译脚本
├── build.sh             # Linux 交叉编译脚本
├── LICENSE              # MIT 许可证
└── README.md            # 本文件
```

## 设置存储

| 数据 | 位置 |
|------|------|
| 用户设置 | 注册表 `HKCU\Software\EyeCare` |
| 开机自启 | 注册表 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` |
| 统计数据 | `%APPDATA%\EyeCare\stats.dat`（纯文本，365 天自动清理） |

注册表键值：

| 键名 | 类型 | 说明 |
|------|------|------|
| Brightness | DWORD | 亮度 0~100 |
| BreakHours | DWORD | 工作时长（小时）0~6 |
| BreakMinutes | DWORD | 工作时长（分钟）0~55 |
| BreakRestSec | DWORD | 休息时长（秒）10~300 |
| FilterMode | DWORD | 滤光模式 0=温和 1=办公 |
| BreakPreset | DWORD | 休息策略 0=自定义 1=20-20-20 |
| EnableFilter | DWORD | 蓝光过滤开关 |
| EnableBreak | DWORD | 定时休息开关 |

## 技术栈

| 项目 | 技术 |
|------|------|
| 语言 | C++17 |
| GUI | 纯 Win32 API，零框架依赖 |
| 绘图 | GDI+ 双缓冲 |
| 字体 | 微软雅黑（中文）+ Consolas（英文/数字） |
| 体积 | ~300KB（MSVC 静态链接） |
| 兼容 | Windows 7 / 8 / 8.1 / 10 / 11 |

## 注意事项

1. `SetDeviceGammaRamp` 在某些显卡驱动上可能不工作
2. 远程桌面连接下无法修改 Gamma Ramp
3. 退出程序时自动恢复原始 Gamma 值
4. 程序异常退出时 Gamma 可能未恢复，重启程序即可
5. MinGW 编译的 exe 可能被杀软误报，建议用 MSVC 编译

## 许可证

[MIT License](LICENSE)
