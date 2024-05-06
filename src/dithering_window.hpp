#pragma once
#include <nanogui/nanogui.h>

class XStitchEditorApplication;

class DitheringWindow : public nanogui::Window {
public:
    DitheringWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();
    void reset_form();

private:
    XStitchEditorApplication *_app;

    nanogui::Label *_errors;
    nanogui::ComboBox *_algorithm_combobox;
    std::vector<nanogui::CheckBox*> _palette_checkboxes;
    nanogui::CheckBox *_enable_max_threads_checkbox;
    nanogui::Label *_max_threads_label;
    nanogui::IntBox<int> *_max_threads_intbox;
};