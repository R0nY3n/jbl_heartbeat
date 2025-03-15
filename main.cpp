#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <cmath>
#include <shlobj.h>
#include "resource.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// 定义使用Unicode版本的Windows API
#ifndef UNICODE
#define UNICODE
#endif

// 资源ID
#define IDI_ICON_RUNNING 101
#define IDI_ICON_PAUSED 102
#define IDR_TRAY_MENU 103
#define ID_TRAY_EXIT 104
#define ID_TRAY_PLAY_TEST 105
#define ID_TRAY_START_STOP 106
#define ID_TRAY_INCREASE 107
#define ID_TRAY_DECREASE 108
#define ID_TRAY_STATUS 109
#define ID_TRAY_AUTOSTART 110
#define WM_TRAYICON (WM_USER + 1)

// 全局变量
HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;
NOTIFYICONDATAW g_nid = {};
std::atomic<bool> g_isRunning(true);
std::atomic<int> g_intervalSeconds(60);
std::thread g_heartbeatThread;
std::mutex g_mutex;
std::condition_variable g_cv;
bool g_threadShouldExit = false;
bool g_autoStart = false;

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitTrayIcon(HWND hWnd, bool isRunning);
void UpdateTrayIcon(bool isRunning);
void UpdateTrayMenu(HMENU hMenu);
void PlayHeartbeatSound();
void HeartbeatThreadFunc();
bool IsAutoStartEnabled();
void SetAutoStart(bool enable);

// 获取当前可执行文件路径
std::wstring GetExecutablePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return path;
}

// 检查是否已设置开机自启动
bool IsAutoStartEnabled() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t value[MAX_PATH];
    DWORD valueSize = sizeof(value);
    DWORD type;
    bool result = false;

    if (RegQueryValueExW(hKey, L"JBLHeartbeat", NULL, &type, (LPBYTE)value, &valueSize) == ERROR_SUCCESS) {
        result = true;
    }

    RegCloseKey(hKey);
    return result;
}

// 设置或取消开机自启动
void SetAutoStart(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return;
    }

    if (enable) {
        std::wstring path = GetExecutablePath();
        RegSetValueExW(hKey, L"JBLHeartbeat", 0, REG_SZ, (BYTE*)path.c_str(), (DWORD)((path.length() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(hKey, L"JBLHeartbeat");
    }

    RegCloseKey(hKey);
    g_autoStart = enable;
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;

    // 初始化Common Controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // 初始化COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // 检查是否已设置开机自启动
    g_autoStart = IsAutoStartEnabled();

    // 注册窗口类
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"JBLHeartbeatClass";
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON_RUNNING));
    RegisterClassExW(&wc);

    // 创建隐藏窗口
    g_hWnd = CreateWindowExW(
        0, L"JBLHeartbeatClass", L"JBL Heartbeat",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300, NULL, NULL, hInstance, NULL);

    if (!g_hWnd) {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // 隐藏窗口
    ShowWindow(g_hWnd, SW_HIDE);

    // 初始化系统托盘图标
    InitTrayIcon(g_hWnd, g_isRunning);

    // 启动心跳线程
    g_heartbeatThread = std::thread(HeartbeatThreadFunc);

    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_threadShouldExit = true;
        g_cv.notify_all();
    }

    if (g_heartbeatThread.joinable()) {
        g_heartbeatThread.join();
    }

    // 移除托盘图标
    Shell_NotifyIconW(NIM_DELETE, &g_nid);

    CoUninitialize();
    return (int)msg.wParam;
}

// 窗口过程
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);

                HMENU hMenu = CreatePopupMenu();
                if (hMenu) {
                    UpdateTrayMenu(hMenu);

                    // 确保窗口是前台窗口，否则弹出菜单后会立即消失
                    SetForegroundWindow(hwnd);

                    // 显示弹出菜单
                    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                                  pt.x, pt.y, 0, hwnd, NULL);

                    DestroyMenu(hMenu);
                }
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_EXIT:
                    DestroyWindow(hwnd);
                    return 0;

                case ID_TRAY_PLAY_TEST:
                    PlayHeartbeatSound();
                    return 0;

                case ID_TRAY_START_STOP:
                    g_isRunning = !g_isRunning;
                    UpdateTrayIcon(g_isRunning);
                    if (g_isRunning) {
                        g_cv.notify_all();
                    }
                    return 0;

                case ID_TRAY_INCREASE:
                    g_intervalSeconds += 5;
                    if (g_intervalSeconds > 300) g_intervalSeconds = 300; // 最大5分钟
                    UpdateTrayIcon(g_isRunning);
                    return 0;

                case ID_TRAY_DECREASE:
                    g_intervalSeconds -= 5;
                    if (g_intervalSeconds < 5) g_intervalSeconds = 5; // 最小5秒
                    UpdateTrayIcon(g_isRunning);
                    return 0;

                case ID_TRAY_AUTOSTART:
                    SetAutoStart(!g_autoStart);
                    return 0;
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 初始化系统托盘图标
void InitTrayIcon(HWND hWnd, bool isRunning) {
    ZeroMemory(&g_nid, sizeof(NOTIFYICONDATAW));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;

    // 设置图标 - 直接从资源加载
    g_nid.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(isRunning ? IDI_ICON_RUNNING : IDI_ICON_PAUSED));
    if (!g_nid.hIcon) {
        // 如果从资源加载失败，尝试从文件加载
        g_nid.hIcon = (HICON)LoadImageW(
            NULL,
            isRunning ? L"alert_running.ico" : L"alert_paused.ico",
            IMAGE_ICON,
            16, 16,
            LR_LOADFROMFILE
        );
    }

    // 设置提示文本
    wcscpy_s(g_nid.szTip, L"JBL Heartbeat");

    // 添加托盘图标
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

// 更新托盘图标
void UpdateTrayIcon(bool isRunning) {
    // 更新图标
    if (g_nid.hIcon) {
        DestroyIcon(g_nid.hIcon);
    }

    // 设置图标 - 直接从资源加载
    g_nid.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(isRunning ? IDI_ICON_RUNNING : IDI_ICON_PAUSED));
    if (!g_nid.hIcon) {
        // 如果从资源加载失败，尝试从文件加载
        g_nid.hIcon = (HICON)LoadImageW(
            NULL,
            isRunning ? L"alert_running.ico" : L"alert_paused.ico",
            IMAGE_ICON,
            16, 16,
            LR_LOADFROMFILE
        );
    }

    // 更新提示文本
    std::wstring tipText = L"JBL Heartbeat - ";
    tipText += isRunning ? L"Running" : L"Paused";
    tipText += L" - Interval: " + std::to_wstring(g_intervalSeconds) + L"s";
    wcscpy_s(g_nid.szTip, tipText.c_str());

    // 更新托盘图标
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// 更新托盘菜单
void UpdateTrayMenu(HMENU hMenu) {
    std::wstring statusText = L"Interval: " + std::to_wstring(g_intervalSeconds) + L" seconds";
    std::wstring startStopText = g_isRunning ? L"Pause" : L"Start";
    std::wstring autoStartText = g_autoStart ? L"✓ Start with Windows" : L"Start with Windows";

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_STATUS, statusText.c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_INCREASE, L"Increase Interval (+5s)");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_DECREASE, L"Decrease Interval (-5s)");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_PLAY_TEST, L"Play Test Sound");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_START_STOP, startStopText.c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_AUTOSTART, autoStartText.c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
}

// 播放心跳声音
void PlayHeartbeatSound() {
    // 生成低频音（50Hz正弦波）
    const int sampleRate = 44100;
    const double frequency = 50.0; // 50Hz低频
    const double amplitude = 0.5;  // 音量
    const double duration = 0.5;   // 0.5秒
    const int numSamples = static_cast<int>(sampleRate * duration);

    // 创建WAV文件头
    struct WavHeader {
        // RIFF块
        char riffId[4] = {'R', 'I', 'F', 'F'};
        uint32_t riffSize;
        char waveId[4] = {'W', 'A', 'V', 'E'};

        // fmt子块
        char fmtId[4] = {'f', 'm', 't', ' '};
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 1; // PCM
        uint16_t numChannels = 1; // 单声道
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample = 16;

        // data子块
        char dataId[4] = {'d', 'a', 't', 'a'};
        uint32_t dataSize;
    };

    WavHeader header;
    header.sampleRate = sampleRate;
    header.numChannels = 1;
    header.bitsPerSample = 16;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;
    header.byteRate = header.sampleRate * header.blockAlign;
    header.dataSize = numSamples * header.blockAlign;
    header.riffSize = 36 + header.dataSize;

    // 创建临时文件
    char tempPath[MAX_PATH];
    char tempFileName[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    GetTempFileNameA(tempPath, "jbl", 0, tempFileName);

    // 写入WAV文件
    FILE* file;
    if (fopen_s(&file, tempFileName, "wb") == 0) {
        // 写入头部
        fwrite(&header, sizeof(header), 1, file);

        // 写入音频数据
        for (int i = 0; i < numSamples; i++) {
            double t = static_cast<double>(i) / sampleRate;
            double value = amplitude * sin(2.0 * 3.14159265358979323846 * frequency * t);
            int16_t sample = static_cast<int16_t>(value * 32767);
            fwrite(&sample, sizeof(int16_t), 1, file);
        }

        fclose(file);

        // 播放声音
        PlaySoundA(tempFileName, NULL, SND_FILENAME);

        // 删除临时文件
        remove(tempFileName);
    }
}

// 心跳线程函数
void HeartbeatThreadFunc() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(g_mutex);
            if (g_threadShouldExit) {
                break;
            }

            if (!g_isRunning) {
                // 等待直到被通知或超时
                g_cv.wait(lock, []{ return g_isRunning.load() || g_threadShouldExit; });
                if (g_threadShouldExit) {
                    break;
                }
            }
        }

        if (g_isRunning) {
            PlayHeartbeatSound();

            // 等待指定的间隔时间
            std::unique_lock<std::mutex> lock(g_mutex);
            g_cv.wait_for(lock, std::chrono::seconds(g_intervalSeconds.load()),
                []{ return !g_isRunning.load() || g_threadShouldExit; });
        }
    }
}