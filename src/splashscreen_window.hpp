#pragma once
#include <nanogui/nanogui.h>

class XStitchEditorApplication;

class SplashScreenWindow : public nanogui::Window {
public:
    SplashScreenWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();

private:
    XStitchEditorApplication *_app;
};