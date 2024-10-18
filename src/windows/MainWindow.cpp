#include "MainWindow.h"
#include "ui/Button.h"
#include "version.h"

#include <tchar.h>
#include <shellapi.h>
#include "Utils.h"

#define CPPSHOT_VERSION L"" PROJECT_NAME " " PROJECT_VERSION " - build: " __DATE__ " " __TIME__

MainWindow::MainWindow() : Window((HBRUSH) (COLOR_BTNFACE + 1), L"MainCreWindow", CPPSHOT_VERSION) {
    setSize(544, 375);
    
    this->addButton()
        .setPosition(10, 10)
        .setSize(500, 50)
        .setTitle(L"This button doesn't do anything, press CTRL+B to take a screenshot");

    this->addButton()
        .setPosition(10, 70)
        .setSize(500, 50)
        .setTitle(L"Or you can press CTRL+SHIFT+B to take inactive and active screenshots");

    this->addButton()
        .setPosition(10, 130)
        .setSize(500, 50)
        .setTitle(L"Or you can press CTRL+ALT+B to take a screenshot of the full screen");

    this->addButton()
        .setPosition(10, 190)
        .setSize(500, 50)
        .setTitle(L"Or you can press CTRL+ALT+X to hide this window");

    this->addButton()
        .setCallback([this](){
            onOpenExplorer();
        })
        .setPosition(10, 300)
        .setSize(200, 30)
        .setTitle(L"Open Screenshots Folder");
}

void MainWindow::onOpenExplorer() {
    ShellExecute(NULL, L"open", L"explorer", CppShot::getSaveDirectory().c_str(), NULL, SW_SHOWNORMAL);
    
}