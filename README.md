护眼助手 v5.0

轻量级蓝光过滤 & 休息提醒小工具
整体不到1MB

## 展示图
<img width="1080" height="870" alt="image" src="https://github.com/user-attachments/assets/9b462793-9613-4538-a40f-088baf1384a6" />
<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/15311650-2613-4919-8643-4ced2cc93425" />



## 功能

1. **蓝光过滤**
   - 两种模式：温和 / 办公
   - 亮度滑块实时调节，即改即生效

2. **定时休息**
   - 自定义策略：自由设置间隔时长
   - 20-20-20 预设：每 20 分钟休息 20 秒
   - 全屏休息屏：像素时钟倒计时 + 进度条
   - 休息结束气泡通知 + 声音提醒

3. **使用时间统计**
   - 实时显示已使用时长
   - 显示距下次休息倒计时

4. **开机自启**
   - 一键开关，写入 HKCU，无需管理员权限

5. **系统托盘**
   - 最小化到托盘，右键菜单快速操作
   - 悬停显示当前状态

## 编译方法

### 方法一：MSVC（推荐，误报率最低）
1. 打开 "x86 Native Tools Command Prompt for VS 2022"
2. cd 到本目录
3. 运行 `build.bat`

## 文件说明

- `EyeCare.cpp` - 主源码
- `resource.h` - 资源 ID 定义
- `EyeCare.rc` - 资源脚本（含版本信息）
- `EyeCare.manifest` - 应用清单（UAC + 兼容性 + DPI）
- `build.bat` - MSVC 编译脚本
- `README.md` - 本文件

## 设置存储

- 注册表路径：`HKEY_CURRENT_USER\Software\EyeCare`
- 开机自启：`HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`
  - 键名：EyeCare

## 技术

- 语言：C++17
- GUI：纯 Win32 API，零框架依赖
- 绘图：GDI+ 双缓冲，无闪烁
- 体积：~50KB

## 注意事项

1. `SetDeviceGammaRamp` 在某些显卡驱动上可能不工作
2. 远程桌面连接下无法修改 Gamma Ramp
3. 退出程序时自动恢复原始 Gamma 值
4. 程序异常退出时 Gamma 可能未恢复，重启程序即可
5. MinGW 编译的 exe 可能被杀软误报，建议用 MSVC 编译

## 开源


