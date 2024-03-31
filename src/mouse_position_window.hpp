#pragma once
#include <nanogui/nanogui.h>
#include "threads.hpp"

class MousePositionWindow : public nanogui::Window {
public:
    MousePositionWindow(nanogui::Widget *parent) : nanogui::Window(parent, "") {};
    void initialise();
    void set_captions(int stitch_x, int stitch_y, int offset, Thread *thread = nullptr);

private:
    nanogui::Label *_mouse_location_label;
    nanogui::Label *_mouse_location_label_2;
};