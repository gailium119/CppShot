#include "BackdropWindow.h"
#include "Utils.h"

#include <tchar.h>
#include <iostream>

void BackdropWindow::waitForResize(LONG left, LONG top, LONG right, LONG bottom) const {

    for(int x = 0; x < 66; x++){ //capping out at 330 ms, which is already fairly slow
    
    	COLORREF currentColor1 = GetPixel(GetDC(HWND_DESKTOP), left, top);
        COLORREF currentColor2 = GetPixel(GetDC(HWND_DESKTOP), left, bottom);
        COLORREF currentColor3 = GetPixel(GetDC(HWND_DESKTOP), right, top);
        COLORREF currentColor4 = GetPixel(GetDC(HWND_DESKTOP), right, bottom);
        std::cout << std::hex << currentColor1 << " " 
            << currentColor2 << " "
            << currentColor3 << " "
            << currentColor4 << " "
            << m_color << std::endl;
        if(m_color == currentColor1 || m_color == currentColor2
        || m_color == currentColor3 || m_color == currentColor4)
            break;

        Sleep(5);
    }
}

BackdropWindow::BackdropWindow(COLORREF color, const TCHAR* className) : Window(CreateSolidBrush(color), className, _T("Backdrop Window"), WS_EX_TOOLWINDOW, WS_POPUP) {
	m_color = color;
}

Window& BackdropWindow::resize(HWND window) {
    m_rect = CppShot::getCaptureRect(window);

    if(!SetWindowPos(m_window, window, m_rect.left, m_rect.top, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top, SWP_NOACTIVATE)){
        SetWindowPos(m_window, NULL, m_rect.left, m_rect.top, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top, SWP_NOACTIVATE);
    }

	return *this;
}

Window& BackdropWindow::show(int nCmdShow) const {
	ShowWindow (m_window, SW_SHOWNOACTIVATE);
    waitForResize(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
    return *const_cast<BackdropWindow*>(this);
}