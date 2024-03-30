#pragma once
#include <nanogui/nanogui.h>
#include "constants.hpp"
using nanogui::Vector2i;

class DisabledButton : public nanogui::Button {
public:
    DisabledButton(nanogui::Widget *parent, const std::string &caption = "Untitled", int icon = 0) : nanogui::Button(parent, caption, icon) {};

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
    bool mouse_over(int x, int y);
private:
    void update_selected_thread_widget();

    DisabledButton *_selected_thread_button;
    nanogui::Label *_selected_thread_label;
};