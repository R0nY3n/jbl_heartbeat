# JBL Heartbeat

一个简单的系统托盘工具，用于防止JBL等品牌音响自动进入休眠状态。

## 功能特点

- 定期发送低频音（40Hz）以保持音响处于活动状态
- 音频持续时间、音量和间隔时间均可自定义
- 系统托盘图标显示当前状态（运行/暂停）
- 右键菜单可以：
  - 播放测试音
  - 开始/暂停发送
  - 调整所有设置参数（间隔、持续时间、音量）
  - 设置开机自启动
  - 访问GitHub项目页面
  - 退出程序

## 使用方法

1. 运行程序，它会自动在系统托盘区域显示图标
2. 默认情况下，程序会每60秒发送一次低频音
3. 右键点击托盘图标可以调整设置或退出程序
4. 点击"Settings"展开设置菜单，可以调整所有参数
5. 选择"Start with Windows"选项可以设置开机自启动
6. 点击"Visit GitHub Page"可以访问项目的GitHub页面

## 可配置参数

- **间隔时间**：两次心跳声音之间的间隔（5-300秒，默认60秒）
- **持续时间**：每次心跳声音的持续时间（0.5-10秒，默认2秒）
- **音量**：心跳声音的音量（10%-100%，默认70%）

所有参数都可以通过托盘菜单中的设置选项进行调整，每个参数都有 [-] 和 [+] 按钮用于增减值。

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

## 源代码

本项目的源代码托管在GitHub上：[https://github.com/R0nY3n/jbl_heartbeat](https://github.com/R0nY3n/jbl_heartbeat)

## 许可证

本项目基于MIT许可证开源。
