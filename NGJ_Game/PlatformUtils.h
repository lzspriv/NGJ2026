#pragma once

// 跨平台的 API：嘗試取得視窗所在顯示器的矩形（桌面座標）或工作區（不含工作列）
// nativeWindowHandle: Raylib 的 GetWindowHandle() 回傳值（在 Windows 為 HWND）
// left/top/width/height 皆為輸出參數
// 回傳 true 表示在本平台成功取得，false 表示失敗（呼叫端應退回使用 Raylib 的 GetCurrentMonitor）

bool GetMonitorRectForWindow(void* nativeWindowHandle, int &left, int &top, int &width, int &height);

// 取得顯示器的工作區（work area，不含 Windows 任務列/停靠欄）
bool GetMonitorWorkAreaForWindow(void* nativeWindowHandle, int &left, int &top, int &width, int &height);
