#pragma once
#include <nanogui/nanogui.h>

class NewProjectWindow : public nanogui::Window {
public:
    NewProjectWindow(nanogui::Widget *parent) : nanogui::Window(parent, "") {};
    void initialise();
    void reset_form();

private:
    nanogui::Label *_errors;
    nanogui::TextBox *_title_entry;
    nanogui::IntBox<int> *_width_entry;
    nanogui::IntBox<int> *_height_entry;
    nanogui::ColorPicker *_color_picker;
};