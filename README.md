# JBL Heartbeat

一个简单的系统托盘工具，用于防止JBL等品牌音响自动进入休眠状态。

## 功能特点

- 定期发送低频音（40Hz）以保持音响处于活动状态
- 音频持续时间为2秒，带有淡入淡出效果，减少爆音
- 可自定义发送间隔时间（默认60秒）
- 系统托盘图标显示当前状态（运行/暂停）
- 右键菜单可以：
  - 增加/减少间隔时间（±5秒）
  - 播放测试音
  - 开始/暂停发送
  - 设置开机自启动
  - 退出程序

## 使用方法

1. 运行程序，它会自动在系统托盘区域显示图标
2. 默认情况下，程序会每60秒发送一次低频音
3. 右键点击托盘图标可以调整设置或退出程序
4. 选择"Start with Windows"选项可以设置开机自启动

## 音频参数

- 频率：40Hz（低频，有效唤醒音响）
- 音量：70%（适中音量，避免过大或过小）
- 持续时间：2秒（足够长的时间确保设备被唤醒）
- 淡入淡出：0.1秒（减少爆音，保护音响）

## 构建方法

本项目使用CMake构建系统。按照以下步骤构建：

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

或者直接使用Visual Studio的开发者命令提示符：

```bash
cl /EHsc /W4 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS main.cpp /link user32.lib shell32.lib comctl32.lib winmm.lib ole32.lib advapi32.lib /SUBSYSTEM:WINDOWS /OUT:jbl_heartbeat.exe
```

## 系统要求

- Windows 7 或更高版本
- 支持音频输出的设备

## 许可证

本项目基于MIT许可证开源。
