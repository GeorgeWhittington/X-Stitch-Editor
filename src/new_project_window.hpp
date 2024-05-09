#pragma once
#include <nanogui/nanogui.h>

class XStitchEditorApplication;

class NewProjectWindow : public nanogui::Window {
public:
    NewProjectWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();
    void reset_form();

private:
    XStitchEditorApplication *_app;

    nanogui::Label *_errors;
    nanogui::TextBox *_title_entry;
    nanogui::IntBox<int> *_width_entry;
    nanogui::IntBox<int> *_height_entry;
    nanogui::ColorPicker *_color_picker;
};