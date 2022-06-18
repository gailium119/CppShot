#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

#define _WIN32_IE 0x0300
#undef __STRICT_ANSI__

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
#include "Screenshot.h"
#include "CompositeScreenshot.h"

#define CPPSHOT_VERSION L"0.5 - build: " __DATE__ " " __TIME__

#define ERROR_TITLE L"CppShot Error"

#define DEFAULT_SAVE_DIRECTORY L"C:\\test\\"

#define SAVE_INTERMEDIARY_IMAGES 0

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("MainCreWindow");
TCHAR blackBackdropClassName[ ] = _T("BlackBackdropWindow");
TCHAR whiteBackdropClassName[ ] = _T("WhiteBackdropWindow");

inline bool FileExists (const std::wstring& name) {
  std::string name_string(name.begin(), name.end());
  struct stat buffer;
  return (stat (name_string.c_str(), &buffer) == 0);
}

inline unsigned __int64 CurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::wstring GetRegistry(LPCTSTR pszValueName, LPCTSTR defaultValue)
{
    // Try open registry key
    HKEY hKey = NULL;
    LPCTSTR pszSubkey = _T("SOFTWARE\\CppShot");
    if ( RegOpenKey(HKEY_CURRENT_USER, pszSubkey, &hKey) != ERROR_SUCCESS )
    {
        std::cout << "Unable to open registry key" << std::endl;
    }

    // Buffer to store string read from registry
    TCHAR szValue[1024];
    DWORD cbValueLength = sizeof(szValue);

    // Query string value
    if ( RegQueryValueEx(
            hKey,
            pszValueName,
            NULL,
            NULL,
            reinterpret_cast<LPBYTE>(&szValue),
            &cbValueLength)
         != ERROR_SUCCESS )
    {
        std::cout << "Unable to read registry value" << std::endl;
        return std::wstring(defaultValue);
    }

    _tprintf(_T("getregistry: %s\n"), szValue);

    return std::wstring(szValue);
}

std::wstring GetSaveDirectory(){
    return GetRegistry(L"Path", DEFAULT_SAVE_DIRECTORY);
}

const wchar_t* statusString(const Gdiplus::Status status) {
    switch (status) {
        case Gdiplus::Ok: return L"Ok";
        case Gdiplus::GenericError: return L"GenericError";
        case Gdiplus::InvalidParameter: return L"InvalidParameter";
        case Gdiplus::OutOfMemory: return L"OutOfMemory";
        case Gdiplus::ObjectBusy: return L"ObjectBusy";
        case Gdiplus::InsufficientBuffer: return L"InsufficientBuffer";
        case Gdiplus::NotImplemented: return L"NotImplemented";
        case Gdiplus::Win32Error: return L"Win32Error";
        case Gdiplus::Aborted: return L"Aborted";
        case Gdiplus::FileNotFound: return L"FileNotFound";
        case Gdiplus::ValueOverflow: return L"ValueOverflow";
        case Gdiplus::AccessDenied: return L"AccessDenied";
        case Gdiplus::UnknownImageFormat: return L"UnknownImageFormat";
        case Gdiplus::FontFamilyNotFound: return L"FontFamilyNotFound";
        case Gdiplus::FontStyleNotFound: return L"FontStyleNotFound";
        case Gdiplus::NotTrueTypeFont: return L"NotTrueTypeFont";
        case Gdiplus::UnsupportedGdiplusVersion: return L"UnsupportedGdiplusVersion";
        case Gdiplus::GdiplusNotInitialized: return L"GdiplusNotInitialized";
        case Gdiplus::PropertyNotFound: return L"PropertyNotFound";
        case Gdiplus::PropertyNotSupported: return L"PropertyNotSupported";
        default: return L"Status Type Not Found.";
    }
}

void DisplayGdiplusStatusError(const Gdiplus::Status status){
    if(status == Gdiplus::Ok)
        return;
    wchar_t errorText[2048];
    _stprintf(errorText, L"An error has occured while saving: %s", statusString(status));
    MessageBox(NULL, errorText, ERROR_TITLE, 0x40010);
}

HWND createBackdropWindow(HINSTANCE hThisInstance, TCHAR className, HBRUSH backgroundBrush){
    HWND hwnd;               /* This is the handle for our window */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = &className;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = backgroundBrush;

    if (!RegisterClassEx (&wincl)){
        MessageBox(NULL, L"Unable to create backdrop window, the program may not work correctly.", ERROR_TITLE, 0x30);
        return NULL;
    }

    hwnd = CreateWindowEx (
           WS_EX_TOOLWINDOW,                   /* Extended possibilites for variation */
            &className,         /* Classname */
           _T("Backdrop Window"),       /* Title Text */
           WS_POPUP , /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           544,                 /* The programs width */
           375,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    //SetWindowLong(hwnd, GWL_STYLE, WS_POPUP);

    return hwnd;
}

HBITMAP CaptureScreenArea(RECT rct){
    //TODO: migrate this away

    HDC hdc = GetDC(HWND_DESKTOP);
    HDC memdc = CreateCompatibleDC(hdc);
    HBITMAP hbitmap = CreateCompatibleBitmap(hdc, rct.right - rct.left, rct.bottom - rct.top);

    SelectObject(memdc, hbitmap);
    BitBlt(memdc, 0, 0, rct.right - rct.left, rct.bottom - rct.top, hdc, rct.left, rct.top, SRCCOPY );

    DeleteDC(memdc);
    ReleaseDC(HWND_DESKTOP, hdc);

    return hbitmap;
}

void WaitForColor(RECT rct, unsigned long color){
    for(int x = 0; x < 66; x++){ //capping out at 330 ms, which is already fairly slow

        RECT rctOnePx;
        rctOnePx.left = rct.left;
        rctOnePx.top = rct.top;
        rctOnePx.right = rct.left + 1;
        rctOnePx.bottom = rct.top + 1;
        HBITMAP pixelBmp = CaptureScreenArea(rctOnePx);

        //code adapted from https://stackoverflow.com/questions/26233848/c-read-pixels-with-getdibits
        HDC hdc = GetDC(0);

        BITMAPINFO MyBMInfo = {0};
        MyBMInfo.bmiHeader.biSize = sizeof(MyBMInfo.bmiHeader);

        // Get the BITMAPINFO structure from the bitmap
        if(0 == GetDIBits(hdc, pixelBmp, 0, 0, NULL, &MyBMInfo, DIB_RGB_COLORS)) {
            std::cout << "error" << std::endl;
        }

        // create the bitmap buffer
        BYTE* lpPixels = new BYTE[MyBMInfo.bmiHeader.biSizeImage];

        // Better do this here - the original bitmap might have BI_BITFILEDS, which makes it
        // necessary to read the color table - you might not want this.
        MyBMInfo.bmiHeader.biCompression = BI_RGB;

        // get the actual bitmap buffer
        if(0 == GetDIBits(hdc, pixelBmp, 0, MyBMInfo.bmiHeader.biHeight, (LPVOID)lpPixels, &MyBMInfo, DIB_RGB_COLORS)) {
            std::cout << "error2" << std::endl;
        }

        //end of stackoverflow code
        unsigned long currentColor = (((unsigned long)lpPixels[0]) << 16) | (((unsigned long)lpPixels[1]) << 8) | (((unsigned long)lpPixels[2]));

        std::cout << currentColor << std::endl;
        if(color == currentColor)
            break;

        Sleep(5);
    }
}

void RemoveIllegalChars(std::wstring* str){
    std::wstring::iterator it;
    std::wstring illegalChars = L"\\/:?\"<>|*";
    for (it = str->begin() ; it < str->end() ; ++it){
        bool found = illegalChars.find(*it) != std::string::npos;
        if(found){
            *it = ' ';
        }
    }
}

void CaptureCompositeScreenshot(HINSTANCE hThisInstance, HWND whiteHwnd, HWND blackHwnd, bool creMode){

    std::cout << "Screenshot capture start: " << CurrentTimestamp() << std::endl;

    HWND desktopWindow = GetDesktopWindow();
    HWND foregroundWindow = GetForegroundWindow();
    HWND taskbar = FindWindow(L"Shell_TrayWnd", NULL);
    HWND startButton = FindWindow(L"Button", L"Start");

    //hiding the taskbar in case it gets in the way
    //note that this may cause issues if the program crashes during capture
    if(foregroundWindow != taskbar && foregroundWindow != startButton){
        ShowWindow(taskbar, 0);
        ShowWindow(startButton, 0);
    }

    SetForegroundWindow(foregroundWindow);

    //calculating screenshot area
    RECT rct;
    RECT rctDesktop;

    GetWindowRect(foregroundWindow, &rct);
    GetWindowRect(desktopWindow, &rctDesktop);

    std::cout << rct.left << ";" << rct.right << ";" << rct.top << ";" << rct.bottom << std::endl;
    std::cout << rctDesktop.left << ";" << rctDesktop.right << ";" << rctDesktop.top << ";" << rctDesktop.bottom << std::endl;

    rct.left = (rctDesktop.left < (rct.left-100)) ? (rct.left - 100) : rctDesktop.left;
    rct.right = (rctDesktop.right > (rct.right+100)) ? (rct.right + 100) : rctDesktop.right;
    rct.bottom = (rctDesktop.bottom > (rct.bottom+100)) ? (rct.bottom + 100) : rctDesktop.bottom;
    rct.top = (rctDesktop.top < (rct.top-100)) ? (rct.top - 100) : rctDesktop.top;

    //spawning backdrop
    if(!SetWindowPos(blackHwnd, foregroundWindow, rct.left, rct.top, rct.right - rct.left, rct.bottom - rct.top, SWP_NOACTIVATE)){
        SetWindowPos(blackHwnd, NULL, rct.left, rct.top, rct.right - rct.left, rct.bottom - rct.top, SWP_NOACTIVATE);
        SetForegroundWindow(foregroundWindow);
    }
    SetWindowPos(whiteHwnd, blackHwnd, rct.left, rct.top, rct.right - rct.left, rct.bottom - rct.top, SWP_NOACTIVATE);

    std::cout << "Additional white flash: " << CurrentTimestamp() << std::endl;
    ShowWindow (whiteHwnd, SW_SHOWNOACTIVATE);
    WaitForColor(rct, RGB(255,255,255));

    ShowWindow (blackHwnd, SW_SHOWNOACTIVATE);
    ShowWindow (whiteHwnd, 0);

    //taking the screenshot
    WaitForColor(rct, RGB(0,0,0));

    std::cout << "Capturing black: " << CurrentTimestamp() << std::endl;

    Screenshot blackShot(foregroundWindow);

    ShowWindow (blackHwnd, 0);
    ShowWindow (whiteHwnd, SW_SHOWNOACTIVATE);

    WaitForColor(rct, RGB(255,255,255));

    std::cout << "Capturing white: " << CurrentTimestamp() << std::endl;
    Screenshot whiteShot(foregroundWindow);

    /*if(creMode){
        Sleep(33); //Time for the foreground window to settle
        SetForegroundWindow(desktopWindow);
        Screenshot whiteCreShot(foregroundWindow);
        WaitForColor(rct, RGB(255,255,255));
    }
    std::cout << "Capturing black inactive: " << CurrentTimestamp() << std::endl;
    if(creMode){
        Screenshot blackCreShow(foregroundWindow);

        ShowWindow (blackHwnd, SW_SHOWNOACTIVATE);
        ShowWindow (whiteHwnd, 0);
        WaitForColor(rct, RGB(0,0,0));
    }
    std::cout << "Capturing white inactive: " << CurrentTimestamp() << std::endl;
    Gdiplus::Bitmap blackInactiveShot(CaptureScreenArea(rct), NULL);*/

    //activating taskbar
    ShowWindow(taskbar, 1);
    ShowWindow(startButton, 1);

    //hiding backdrop
    ShowWindow (blackHwnd, 0);
    ShowWindow (whiteHwnd, 0);

    //differentiating alpha
    std::cout << "Differentiating alpha: " << CurrentTimestamp() << std::endl;
    CompositeScreenshot transparentImage(whiteShot, blackShot);
    /*if(creMode)
        DifferentiateAlpha(&whiteInactiveShot, &blackInactiveShot, &transparentInactiveBitmap);*/

    //calculating crop
    /*std::cout << "Capturing crop: " << CurrentTimestamp() << std::endl;
    Gdiplus::Rect crop = CalculateCrop(&transparentBitmap);
    if(crop.GetLeft() == crop.GetRight() || crop.GetTop() == crop.GetBottom()){
        ShowWindow (whiteHwnd, 0);
        ShowWindow (blackHwnd, 0);
        MessageBox(whiteHwnd, L"Screenshot is empty, aborting capture.", L"Error", MB_OK | MB_ICONSTOP);
        return;
    }

    std::cout << "Creating bitmaps: " << CurrentTimestamp() << std::endl;
    CompositeScreenshot transparentImage();*/

    //Saving the image
    std::cout << "Saving: " << CurrentTimestamp() << std::endl;

    TCHAR h[2048];
    GetWindowText(foregroundWindow, h, 2048);
    std::wstring windowTextStr(h);

    RemoveIllegalChars(&windowTextStr);

    std::wcout << windowTextStr << std::endl;
    //std::cout << std::endl << len;

    std::wstring path = GetSaveDirectory();
    std::wcout << L"registrypath: " << path << std::endl;

    CreateDirectory(path.c_str(), NULL);
    std::wstringstream pathbuild;
    std::wstringstream pathbuildInactive;
    pathbuild << path << L"\\" << windowTextStr << L"_b1.png";
    pathbuildInactive << path << L"\\" << windowTextStr << L"_b2.png";

    std::wstring fileName = pathbuild.str();
    std::wstring fileNameInactive = pathbuildInactive.str();

    unsigned int i = 0;
    while(FileExists(fileName) | FileExists(fileNameInactive)){
        pathbuild.str(L"");
        pathbuildInactive.str(L"");
        pathbuild << path << L"\\" << windowTextStr << L"_" << i << L"_b1.png";
        pathbuildInactive << path << L"\\" << windowTextStr << L"_" << i << L"_b2.png";
        fileName = pathbuild.str();
        fileNameInactive = pathbuildInactive.str();
        i++;
    }

    transparentImage.save(pathbuild.str());

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

int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    INITCOMMONCONTROLSEX icc;

    // Initialise common controls.
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = (HICON) LoadImage(hThisInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_DEFAULTCOLOR | LR_SHARED);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
    wincl.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           _T("CppShot " CPPSHOT_VERSION),       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           544,                 /* The programs width */
           375,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    HWND hwndButton = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed
            L"This button doesn't do anything, press CTRL+B to take a screenshot",      // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
            10,         // x position
            10,         // y position
            500,        // Button width
            100,        // Button height
            hwnd,     // Parent window
            NULL,       // No menu.
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.

     HWND hwndButtonTwo = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed
            L"Or you can press CTRL+SHIFT+B to take inactive and active screenshots",      // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
            10,         // x position
            120,         // y position
            500,        // Button width
            100,        // Button height
            hwnd,     // Parent window
            NULL,       // No menu.
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.

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

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nCmdShow);

    /* Create backdrop windows */
    HWND whiteHwnd = createBackdropWindow(hThisInstance, *whiteBackdropClassName, (HBRUSH) CreateSolidBrush(RGB(255,255,255)));
    HWND blackHwnd = createBackdropWindow(hThisInstance, *blackBackdropClassName, (HBRUSH) CreateSolidBrush(RGB(0,0,0)));

    /* Start GDI+ */
    Gdiplus::GdiplusStartupInput gpStartupInput;
    ULONG_PTR gpToken;
    int val = Gdiplus::GdiplusStartup(&gpToken, &gpStartupInput, NULL);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        if (messages.message == WM_HOTKEY)
        {
            _tprintf(_T("WM_HOTKEY received\n"));
            if (messages.wParam == 1)
                CaptureCompositeScreenshot(hThisInstance, whiteHwnd, blackHwnd, false);
            else if (messages.wParam == 2)
                CaptureCompositeScreenshot(hThisInstance, whiteHwnd, blackHwnd, true);
        }

        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}

VOID StartExplorer()
{
    std::wstring path = GetSaveDirectory();
   // additional information
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   // set the size of the structures
   ZeroMemory( &si, sizeof(si) );
   si.cb = sizeof(si);
   ZeroMemory( &pi, sizeof(pi) );

   TCHAR commandLine[2048];
   _stprintf(commandLine, L"explorer \"%s\"", path.c_str());

  // start the program up
  CreateProcess( NULL,   // the path
    commandLine,        // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    FALSE,          // Set handle inheritance to FALSE
    0,              // No creation flags
    NULL,           // Use parent's environment block
    NULL,           // Use parent's starting directory
    &si,            // Pointer to STARTUPINFO structure
    &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );
    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}

/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;

        case WM_COMMAND:               /* menu items */
          switch (LOWORD(wParam))
          {
            case ID_FILE_OPEN:
            {
              StartExplorer();
              return 0;
            }

            case ID_FILE_EXIT:
            {
              DestroyWindow(hwnd);
              return 0;
            }
          }
          break;

        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}