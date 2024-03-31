#pragma once
#include <functional>
#include <nanogui/nanogui.h>
#include "constants.hpp"
#include "threads.hpp"

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

class PaletteButton : public nanogui::Button {
public:
    PaletteButton(nanogui::Widget *parent, const std::string &caption = "Untitled", int icon = 0) : nanogui::Button(parent, caption, icon) {};
    void set_thread(Thread *thread) { _thread = thread; };
    void set_app(nanogui::Widget *app) { _app = (XStitchEditorApplication*)app; };
    void palettebutton_callback();
    void set_callback() {
        m_callback = [this]() { this->palettebutton_callback(); };;
    };

private:
    Thread *_thread;
    XStitchEditorApplication *_app;
};

class ToolWindow : public nanogui::Window {
public:
    ToolWindow(nanogui::Widget *parent) : nanogui::Window(parent, "Tools") {};
    void initialise();
    bool mouse_over(int x, int y);
    void set_palette();
    void update_selected_thread_widget();

private:
    DisabledButton *_selected_thread_button;
    nanogui::Label *_selected_thread_label;
    nanogui::VScrollPanel *palette_container;
};