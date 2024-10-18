#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <chrono>

namespace CppShot {
    std::wstring getRegistry(LPCTSTR pszValueName, LPCTSTR defaultValue);
    std::wstring getSaveDirectory();
    const wchar_t* statusString(const Gdiplus::Status status);
    RECT getDesktopRect();
    RECT getCaptureRect(HWND window);
    BOOL CALLBACK getMonitorRectsCallback(HMONITOR unnamedParam1, HDC unnamedParam2, LPRECT unnamedParam3, LPARAM unnamedParam4);
    std::vector<RECT> getMonitorRects();
    unsigned int getDPIForWindow(HWND window);

    inline unsigned __int64 currentTimestamp() {
#ifdef _M_ARM
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        // Convert FILETIME (100-nanosecond intervals since Jan 1, 1601) to Unix epoch (milliseconds since Jan 1, 1970)
        ULARGE_INTEGER ull;
        ull.LowPart = ft.dwLowDateTime;
        ull.HighPart = ft.dwHighDateTime;

        // FILETIME is in 100-nanosecond intervals, so convert to milliseconds
        return (ull.QuadPart / 10000ULL) - 11644473600000ULL;
#else
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif
    }
}