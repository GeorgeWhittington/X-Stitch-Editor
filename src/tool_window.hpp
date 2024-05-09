#pragma once
#include <functional>
#include <nanogui/nanogui.h>
#include "constants.hpp"
#include "threads.hpp"

class DisabledButton : public nanogui::Button {
public:
    DisabledButton(nanogui::Widget *parent, const std::string &caption = "Untitled", int icon = 0) : nanogui::Button(parent, caption, icon) {};

    virtual bool mouse_enter_event(const nanogui::Vector2i &p, bool enter) {
        return false;
    };
    virtual bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
        return false;
    };
};

class XStitchEditorApplication;

class PaletteButton : public nanogui::Button {
public:
    PaletteButton(nanogui::Widget *parent, const std::string &caption = "  ", int icon = 0) : nanogui::Button(parent, caption, icon) {};
    void set_thread(Thread *thread) { _thread = thread; };
    void set_app(nanogui::Widget *app) { _app = (XStitchEditorApplication*)app; };
    void palettebutton_callback();
    void set_callback() {
        m_callback = [this]() { this->palettebutton_callback(); };
    };

protected:
    Thread *_thread;
    XStitchEditorApplication *_app;
};

class EditPaletteButton : public PaletteButton {
public:
    EditPaletteButton(nanogui::Widget *parent, const std::string &caption = "  ", int icon = 0) : PaletteButton(parent, caption, icon) {};
    void palettebutton_callback();
    void set_callback() {
        m_callback = [this]() { this->palettebutton_callback(); };
    };
};

class ToolWindow : public nanogui::Window {
public:
    ToolWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "Tools") {};
    void initialise();
    bool mouse_over(nanogui::Vector2i position);
    void set_palette();
    void update_selected_thread_widget();

    nanogui::PopupButton *_add_to_palette_button;

private:
    XStitchEditorApplication *_app;

    DisabledButton *_selected_thread_button;
    nanogui::Label *_selected_thread_label;
    nanogui::VScrollPanel *_palette_container;
};