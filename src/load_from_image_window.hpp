#pragma once
#include <nanogui/nanogui.h>
#include <string>
#ifndef cimg_display
#define cimg_display 0
#endif
#include "CImg.h"

class XStitchEditorApplication;

class LoadFromImageWindow : public nanogui::Window {
public:
    LoadFromImageWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();

private:
    XStitchEditorApplication *_app;
};