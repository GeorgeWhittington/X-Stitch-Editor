#pragma once
#include <nanogui/nanogui.h>

class SplashScreenWindow : public nanogui::Window {
public:
    SplashScreenWindow(nanogui::Widget *parent) : nanogui::Window(parent, "") {};
    void initialise();
};