#pragma once
#include <nanogui/nanogui.h>
#include "constants.hpp"
using nanogui::Vector2i;

class DisabledButton : public nanogui::Button {
public:
    virtual bool mouse_enter_event(const Vector2i &p, bool enter) {
        return false;
    };
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) {
        return false;
    };
};

class XStitchEditorApplication;

class ToolWindow : public nanogui::Window {
public:
    ToolWindow(nanogui::Widget *parent) : nanogui::Window(parent, "Tools") {};
    void initialise();
};