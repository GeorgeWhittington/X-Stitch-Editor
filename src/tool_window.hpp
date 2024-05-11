#pragma once
#include <functional>
#include <nanogui/nanogui.h>
#include "constants.hpp"
#include "threads.hpp"

class ThemeNoRefCount : public nanogui::Theme {
public:
    ThemeNoRefCount(NVGcontext *ctx) : nanogui::Theme(ctx) {};
    virtual ~ThemeNoRefCount() {};
    void inc_ref() const noexcept {};
};

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

class AddToPaletteButton : public PaletteButton {
public:
    AddToPaletteButton(nanogui::Widget *parent, const std::string &caption = "  ", int icon = 0) : PaletteButton(parent, caption, icon) {};
    void palettebutton_callback();
    void set_popup_button(nanogui::PopupButton *button) { _button = button; };
    void set_callback() {
        m_callback = [this]() { this->palettebutton_callback(); };
    };

private:
    nanogui::PopupButton *_button;
};

class DeletePaletteButton : public nanogui::Button {
public:
    DeletePaletteButton(nanogui::Widget *parent, const std::string &caption, int icon = 0) : nanogui::Button(parent, caption, icon) {};
    void set_thread(Thread *thread) { _thread = thread; };
    void set_app(XStitchEditorApplication *app) { _app = app; };
    void palettebutton_callback();
    void set_callback() {
        m_callback = [this]() { this->palettebutton_callback(); };
    };

protected:
    Thread *_thread;
    XStitchEditorApplication *_app;
};

class ToolWindow : public nanogui::Window {
public:
    ToolWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "Tools") {};
    void initialise();
    bool mouse_over(nanogui::Vector2i position);
    void update_palette_widget();
    void update_selected_thread_widget();
    void update_remove_thread_widget();
    void update_thread_list_popups(nanogui::PopupButton *add_thread_btn);
    void create_themes();
    void reset_add_thread_form();

    nanogui::PopupButton *_add_to_palette_button;
    nanogui::PopupButton *_remove_from_palette_button;
    nanogui::Widget *_add_to_palette_widget;
    nanogui::Widget *_remove_from_palette_widget;
    nanogui::Theme *_palettebutton_black_text_theme = nullptr;
    nanogui::Theme *_palettebutton_white_text_theme = nullptr;
    nanogui::PopupButton *_add_thread_popup_button;
    nanogui::PopupButton *_add_blend_thread_popup_button;
    nanogui::Button *_clear_threads_button;
    nanogui::Button *_add_thread_button;
    nanogui::Label *_add_thread_errors;

    SingleThread *_selected_thread = nullptr;
    SingleThread *_selected_blend_thread = nullptr;

private:
    void zoom_in();
    void zoom_out();

    XStitchEditorApplication *_app;

    DisabledButton *_selected_thread_button;
    nanogui::Label *_selected_thread_label;
    nanogui::VScrollPanel *_palette_container;
    nanogui::Theme *_selected_thread_theme;

    nanogui::VScrollPanel *_remove_threads_scroll_panel;
    nanogui::Widget *_remove_threads_widget = nullptr;
};