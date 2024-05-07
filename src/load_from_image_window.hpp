#pragma once
#include <nanogui/nanogui.h>
#include <string>

class XStitchEditorApplication;

class LoadFromImageWindow : public nanogui::Window {
public:
    LoadFromImageWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();

private:
    XStitchEditorApplication *_app;
};