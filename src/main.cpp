#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <gdiplus.h>
#include <sstream>
#include <iostream>
#include <chrono>
#include <string>
#include <sys/stat.h>

#include "resources.h"
#include "Utils.h"
#include "images/Screenshot.h"
#include "images/CompositeScreenshot.h"
#include "windows/MainWindow.h"
#include "windows/BackdropWindow.h"

#define ERROR_TITLE L"CppShot Error"

/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("MainCreWindow");
TCHAR blackBackdropClassName[ ] = _T("BlackBackdropWindow");
TCHAR whiteBackdropClassName[ ] = _T("WhiteBackdropWindow");

inline bool FileExists (const std::wstring& name) {
    
    return GetFileAttributes(name.c_str()) != INVALID_FILE_ATTRIBUTES && GetLastError() != ERROR_FILE_NOT_FOUND;
}

void DisplayGdiplusStatusError(const Gdiplus::Status status){
    if(status == Gdiplus::Ok)
        return;
    wchar_t errorText[2048];
    _stprintf(errorText, L"An error has occured while saving: %s", CppShot::statusString(status));
    MessageBox(NULL, errorText, ERROR_TITLE, 0x40010);
}

void RemoveIllegalChars(std::wstring& str){
    std::wstring::iterator it;
    std::wstring illegalChars = L"\\/:?\"<>|*";
    for (it = str.begin() ; it < str.end() ; ++it){
        bool found = illegalChars.find(*it) != std::string::npos;
        if(found){
            *it = ' ';
        }
    }
}

std::wstring GetSafeFilenameBase(std::wstring windowTitle) {
    RemoveIllegalChars(windowTitle);

    std::wstring path = CppShot::getSaveDirectory();
    std::wcout << L"registrypath: " << path << std::endl;

    CreateDirectory(path.c_str(), NULL);

    std::wstringstream pathbuild;

    std::wstring fileNameBase;

    unsigned int i = 0;
    do {
        pathbuild.str(L"");

        pathbuild << path << L"\\" << windowTitle << L"_" << i;

        fileNameBase = pathbuild.str();

        i++;
    } while(FileExists(fileNameBase + L"_b1.png") || FileExists(fileNameBase + L"_b2.png"));

    return fileNameBase;
}

void CaptureCompositeScreenshot(HINSTANCE hThisInstance, BackdropWindow& whiteWindow, BackdropWindow& blackWindow, bool creMode){
    std::cout << "Screenshot capture start: " << CppShot::currentTimestamp() << std::endl;

    HWND desktopWindow = GetDesktopWindow();
    HWND foregroundWindow = GetForegroundWindow();
    HWND taskbar = FindWindow(L"Shell_TrayWnd", NULL);
    HWND startButton = FindWindow(L"Button", L"Start");

    std::pair<Screenshot, Screenshot> shots;
    std::pair<Screenshot, Screenshot> creShots;

    //hiding the taskbar in case it gets in the way
    //note that this may cause issues if the program crashes during capture
    if(foregroundWindow != taskbar && foregroundWindow != startButton){
        ShowWindow(taskbar, 0);
        ShowWindow(startButton, 0);
    }

    whiteWindow.resize(foregroundWindow);
    blackWindow.resize(foregroundWindow);

    //spawning backdrop
    SetForegroundWindow(foregroundWindow);

    std::cout << "Additional white flash: " << CppShot::currentTimestamp() << std::endl;
    
    //WaitForColor(rct, RGB(255,255,255));
    blackWindow.hide();
    whiteWindow.show();

    //taking the screenshot
    //WaitForColor(rct, RGB(0,0,0));

    std::cout << "Capturing black: " << CppShot::currentTimestamp() << std::endl;


    whiteWindow.hide();
    blackWindow.show();
    
    shots.second.capture(foregroundWindow);

    //WaitForColor(rct, RGB(255,255,255));

    std::cout << "Capturing white: " << CppShot::currentTimestamp() << std::endl;
    blackWindow.hide();
    whiteWindow.show();
    
    shots.first.capture(foregroundWindow);

    if(creMode){
        SetForegroundWindow(desktopWindow);
        Sleep(33); //Time for the foreground window to settle
        creShots.first.capture(foregroundWindow); //order swapped bc were starting with white now
        
        std::cout << "Capturing black inactive: " << CppShot::currentTimestamp() << std::endl;
        whiteWindow.hide();
        blackWindow.show();

        creShots.second.capture(foregroundWindow);
    }

    //activating taskbar
    ShowWindow(taskbar, 1);
    ShowWindow(startButton, 1);

    //hiding backdrop
    blackWindow.hide();
    whiteWindow.hide();

    if(!shots.first.isCaptured() || !shots.second.isCaptured()){
        MessageBox(NULL, L"Screenshot is empty, aborting capture.", ERROR_TITLE, MB_OK | MB_ICONSTOP);
        return;
    }

    //differentiating alpha
    std::cout << "Starting image save: " << CppShot::currentTimestamp() << std::endl;

    //Saving the image
    std::cout << "Saving: " << CppShot::currentTimestamp() << std::endl;

    TCHAR h[2048];
    GetWindowText(foregroundWindow, h, 2048);
    std::wstring windowTextStr(h);
    if (foregroundWindow == taskbar) {
        windowTextStr = L"TaskBar";
    }
    if (windowTextStr.empty()) {
        windowTextStr = L"Unknown";
    }
    //std::cout << std::endl << len;

    auto base = GetSafeFilenameBase(windowTextStr);

    std::cout << "Differentiating alpha: " << CppShot::currentTimestamp() << std::endl;
    try {
        CompositeScreenshot transparentImage(shots.first, shots.second);
        transparentImage.save(base + L"_b1.png");

        if(creShots.first.isCaptured() && creShots.second.isCaptured()){
            CompositeScreenshot transparentInactiveImage(creShots.first, creShots.second, transparentImage.getCrop());
            std::cout << "Inactive image ptr: " << transparentInactiveImage.getBitmap() << std::endl;
            std::cout << transparentInactiveImage.getBitmap()->GetWidth() << "x" << transparentInactiveImage.getBitmap()->GetHeight() << std::endl;
            transparentInactiveImage.save(base + L"_b2.png");
        }
    } catch(std::runtime_error& e) {
        MessageBox(NULL, L"An error has occured while capturing the screenshot.", ERROR_TITLE, MB_OK | MB_ICONSTOP);
        return;
    }

    /*std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring fileNameUtf16 = converter.from_bytes(fileName);
    std::wstring fileNameInactiveUtf16 = converter.from_bytes(fileNameInactive);*/

    /*std::wcout << fileName << std::endl << fileNameInactive << std::endl;
    DisplayGdiplusStatusError(clonedBitmap->Save(fileName.c_str(), &pngEncoder, NULL));
    if(creMode)
        DisplayGdiplusStatusError(clonedInactive->Save(fileNameInactive.c_str(), &pngEncoder, NULL));

    std::cout << "Done: " << CurrentTimestamp() << std::endl;
    //Cleaning memory
    delete clonedBitmap;
    delete clonedInactive;*/

}

void CaptureFullScreenScreenshot(HINSTANCE hThisInstance, BackdropWindow& whiteWindow, BackdropWindow& blackWindow, bool creMode) {
    std::cout << "Screenshot capture start: " << CppShot::currentTimestamp() << std::endl;

    HWND desktopWindow = GetDesktopWindow();
    HWND taskbar = FindWindow(L"Shell_TrayWnd", NULL);
    HWND startButton = FindWindow(L"Button", L"Start");

    std::pair<Screenshot, Screenshot> shots;
    std::pair<Screenshot, Screenshot> creShots;


    whiteWindow.resize(desktopWindow);
    blackWindow.resize(desktopWindow);


    std::cout << "Additional white flash: " << CppShot::currentTimestamp() << std::endl;

    //WaitForColor(rct, RGB(255,255,255));

    //taking the screenshot
    //WaitForColor(rct, RGB(0,0,0));

    std::cout << "Capturing black: " << CppShot::currentTimestamp() << std::endl;


    whiteWindow.hide();
    blackWindow.show();

    blackWindow.hide();
    shots.second.captureDesktop();

    //WaitForColor(rct, RGB(255,255,255));

    std::cout << "Capturing white: " << CppShot::currentTimestamp() << std::endl;
    blackWindow.hide();
    whiteWindow.show();
    whiteWindow.hide();

    shots.first.captureDesktop();

    //hiding backdrop
    blackWindow.hide();
    whiteWindow.hide();

    if (!shots.first.isCaptured() || !shots.second.isCaptured()) {
        MessageBox(NULL, L"Screenshot is empty, aborting capture.", ERROR_TITLE, MB_OK | MB_ICONSTOP);
        return;
    }

    //differentiating alpha
    std::cout << "Starting image save: " << CppShot::currentTimestamp() << std::endl;

    //Saving the image
    std::cout << "Saving: " << CppShot::currentTimestamp() << std::endl;

    std::wstring windowTextStr= L"FullScreen";
    //std::cout << std::endl << len;

    auto base = GetSafeFilenameBase(windowTextStr);

    std::cout << "Differentiating alpha: " << CppShot::currentTimestamp() << std::endl;
    try {
        CompositeScreenshot transparentImage(shots.first, shots.second);
        transparentImage.save(base + L"_b1.png");

        if (creShots.first.isCaptured() && creShots.second.isCaptured()) {
            CompositeScreenshot transparentInactiveImage(creShots.first, creShots.second, transparentImage.getCrop());
            std::cout << "Inactive image ptr: " << transparentInactiveImage.getBitmap() << std::endl;
            std::cout << transparentInactiveImage.getBitmap()->GetWidth() << "x" << transparentInactiveImage.getBitmap()->GetHeight() << std::endl;
            transparentInactiveImage.save(base + L"_b2.png");
        }
    }
    catch (std::runtime_error& e) {
        MessageBox(NULL, L"An error has occured while capturing the screenshot.", ERROR_TITLE, MB_OK | MB_ICONSTOP);
        return;
    }

    /*std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring fileNameUtf16 = converter.from_bytes(fileName);
    std::wstring fileNameInactiveUtf16 = converter.from_bytes(fileNameInactive);*/

    /*std::wcout << fileName << std::endl << fileNameInactive << std::endl;
    DisplayGdiplusStatusError(clonedBitmap->Save(fileName.c_str(), &pngEncoder, NULL));
    if(creMode)
        DisplayGdiplusStatusError(clonedInactive->Save(fileNameInactive.c_str(), &pngEncoder, NULL));

    std::cout << "Done: " << CurrentTimestamp() << std::endl;
    //Cleaning memory
    delete clonedBitmap;
    delete clonedInactive;*/

}
static LONG WINAPI exceptionHandler(LPEXCEPTION_POINTERS info) {
    //restore taskbar and start button

    HWND taskbar = FindWindow(L"Shell_TrayWnd", NULL);
    HWND startButton = FindWindow(L"Button", L"Start");

    ShowWindow(taskbar, 1);
    ShowWindow(startButton, 1);

    std::wstringstream ss;
    ss << L"An unhandled exception has occured. The program will now terminate.\n\n";
    ss << L"Exception code: 0x" << std::hex << info->ExceptionRecord->ExceptionCode << std::dec << L"\n";
    ss << L"Exception address: 0x" << std::hex << info->ExceptionRecord->ExceptionAddress << std::dec << L"\n";
    MessageBox(NULL, ss.str().c_str(), ERROR_TITLE, MB_OK | MB_ICONSTOP);
    return EXCEPTION_CONTINUE_SEARCH;
}

int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
    SetUnhandledExceptionFilter(exceptionHandler);
#ifdef _DEBUG
    FILE* conout;
    AllocConsole();
    freopen_s(
        &conout,
        "CONOUT$",
        "w",
        stdout
    );
#endif
    MainWindow window;
    window.show(nCmdShow);

    if (RegisterHotKey(
        NULL,
        1,
        0x2,
        0x42))  //0x42 is 'b'
    {
        _tprintf(_T("CTRL+b\n"));
    }else{
        MessageBox(NULL, L"Unable to register the CTRL+B keyboard shortcut.", ERROR_TITLE, 0x10);
    }

    if (RegisterHotKey(
        NULL,
        2,
        0x6,
        0x42))  //0x42 is 'b'
    {
        _tprintf(_T("CTRL+SHIFT+b\n"));
    }else{
        MessageBox(NULL, L"Unable to register the CTRL+SHIFT+B keyboard shortcut.", ERROR_TITLE, 0x10);
    }
    if (RegisterHotKey(
        NULL,
        3,
        0x3,
        0x42))  //0x42 is 'b'
    {
        _tprintf(_T("CTRL+ALT+b\n"));
    }
    else {
        MessageBox(NULL, L"Unable to register the CTRL+ALT+B keyboard shortcut.", ERROR_TITLE, 0x10);
    }

    if (RegisterHotKey(
        NULL,
        4,
        0x3,
        0x58))  //0x42 is 'x'
    {
        _tprintf(_T("CTRL+ALT+x\n"));
    }
    else {
        MessageBox(NULL, L"Unable to register the CTRL+ALT+X keyboard shortcut.", ERROR_TITLE, 0x10);
    }
    /* Create backdrop windows */
    BackdropWindow whiteWindow(RGB(255, 255, 255), whiteBackdropClassName);
    BackdropWindow blackWindow(RGB(0, 0, 0), blackBackdropClassName);

    /* Start GDI+ */
    Gdiplus::GdiplusStartupInput gpStartupInput;
    ULONG_PTR gpToken;
    int val = Gdiplus::GdiplusStartup(&gpToken, &gpStartupInput, NULL);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    MSG messages;
    while (GetMessage (&messages, NULL, 0, 0))
    {
        if (messages.message == WM_HOTKEY)
        {
            _tprintf(_T("WM_HOTKEY received\n"));
            if (messages.wParam == 1)
                CaptureCompositeScreenshot(hThisInstance, whiteWindow, blackWindow, false);
            else if (messages.wParam == 2)
                CaptureCompositeScreenshot(hThisInstance, whiteWindow, blackWindow, true);
            else if (messages.wParam == 3)
                CaptureFullScreenScreenshot(hThisInstance, whiteWindow, blackWindow, false);
            else if (messages.wParam == 4)
            {
                nCmdShow ^= 1; window.show(nCmdShow);
            }
        }

        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
