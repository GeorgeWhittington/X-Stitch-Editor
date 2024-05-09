#pragma once
#include <nanogui/nanogui.h>
#include "threads.hpp"

class XStitchEditorApplication;

class MousePositionWindow : public nanogui::Window {
public:
    MousePositionWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();
    void set_captions(int stitch_x, int stitch_y, Thread *thread = nullptr);

private:
    XStitchEditorApplication *_app;

    nanogui::Label *_mouse_location_label;
    nanogui::Label *_mouse_location_label_2;
};