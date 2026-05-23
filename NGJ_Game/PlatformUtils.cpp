#include "PlatformUtils.h"

#if defined(_WIN32)
#include <windows.h>

bool GetMonitorRectForWindow(void* nativeWindowHandle, int &left, int &top, int &width, int &height) {
	if (!nativeWindowHandle) return false;
	HWND hwnd = (HWND)nativeWindowHandle;
	RECT wr;
	if (!GetWindowRect(hwnd, &wr)) return false;

	POINT centerPt;
	centerPt.x = (wr.left + wr.right) / 2;
	centerPt.y = (wr.top + wr.bottom) / 2;

	HMONITOR hMon = MonitorFromPoint(centerPt, MONITOR_DEFAULTTONEAREST);
	if (!hMon) return false;

	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMon, &mi)) return false;

	left = mi.rcMonitor.left;
	top = mi.rcMonitor.top;
	width = mi.rcMonitor.right - mi.rcMonitor.left;
	height = mi.rcMonitor.bottom - mi.rcMonitor.top;
	return true;
}

bool GetMonitorWorkAreaForWindow(void* nativeWindowHandle, int &left, int &top, int &width, int &height) {
	if (!nativeWindowHandle) return false;
	HWND hwnd = (HWND)nativeWindowHandle;
	RECT wr;
	if (!GetWindowRect(hwnd, &wr)) return false;

	POINT centerPt;
	centerPt.x = (wr.left + wr.right) / 2;
	centerPt.y = (wr.top + wr.bottom) / 2;

	HMONITOR hMon = MonitorFromPoint(centerPt, MONITOR_DEFAULTTONEAREST);
	if (!hMon) return false;

	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMon, &mi)) return false;

	// 使用 rcWork 而非 rcMonitor，rcWork 為工作區（不含任務列）
	left = mi.rcWork.left;
	top = mi.rcWork.top;
	width = mi.rcWork.right - mi.rcWork.left;
	height = mi.rcWork.bottom - mi.rcWork.top;
	return true;
}

#else

bool GetMonitorRectForWindow(void* nativeWindowHandle, int &left, int &top, int &width, int &height) {
	// 非 Windows 平台暫不實作，回傳 false
	(void)nativeWindowHandle; (void)left; (void)top; (void)width; (void)height;
	return false;
}

bool GetMonitorWorkAreaForWindow(void* nativeWindowHandle, int &left, int &top, int &width, int &height) {
	(void)nativeWindowHandle; (void)left; (void)top; (void)width; (void)height;
	return false;
}

#endif
