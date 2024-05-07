#pragma once
#include <nanogui/nanogui.h>

class XStitchEditorApplication;

enum DitheringAlgorithms {
    FLOYD_STEINBURG,
    BAYER
};

class DitheringWindow : public nanogui::Window {
public:
    DitheringWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();
    void reset_form();
    void set_input_image(unsigned char *image, int width, int height);

private:
    void create_pattern();

    XStitchEditorApplication *_app;

    nanogui::Label *_errors;
    nanogui::TextBox *_title_entry;
    nanogui::ColorPicker *_color_picker;
    nanogui::ComboBox *_algorithm_combobox;
    std::vector<nanogui::CheckBox*> _palette_checkboxes;
    nanogui::CheckBox *_enable_max_threads_checkbox;
    nanogui::Label *_max_threads_label;
    nanogui::IntBox<int> *_max_threads_intbox;

    unsigned char *_image = nullptr;
    int _width = 0;
    int _height = 0;
};