#pragma once
#include <nanogui/nanogui.h>

class XStitchEditorApplication;

enum DitheringAlgorithms {
    FLOYD_STEINBURG,
    BAYER,
    QUANTISE
};

class DitheringWindow : public nanogui::Window {
public:
    DitheringWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();
    void reset_form();
    void select_image();

private:
    void create_pattern();

    XStitchEditorApplication *_app;

    nanogui::Label *_errors;
    nanogui::TextBox *_title_entry;
    nanogui::IntBox<int> *_width_intbox;
    nanogui::IntBox<int> *_height_intbox;
    nanogui::Button *_aspect_ratio_button;
    nanogui::ColorPicker *_color_picker;
    nanogui::ComboBox *_algorithm_combobox;
    nanogui::Label *_matrix_size_label;
    nanogui::Widget *_matrix_size_widget;
    nanogui::ComboBox *_matrix_size_combobox;
    std::vector<nanogui::CheckBox*> _palette_checkboxes;
    nanogui::CheckBox *_enable_max_threads_checkbox;
    nanogui::Label *_max_threads_label;
    nanogui::IntBox<int> *_max_threads_intbox;

    unsigned char *_image = nullptr;
    int _width = 0;
    int _height = 0;
};