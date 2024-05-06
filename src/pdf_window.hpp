#pragma once
#include <nanogui/nanogui.h>
#include <string>

class XStitchEditorApplication;

class PDFWindow : public nanogui::Window {
public:
    PDFWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "PDF Wizard") {};
    void initialise();
    void reset_form();
    void save_pdf();

private:
    XStitchEditorApplication *_app;

    nanogui::Label *_errors;
    nanogui::CheckBox *_checkbox_bw;
    nanogui::CheckBox *_checkbox_colour;
    nanogui::TextBox *_author_textbox;
    nanogui::CheckBox *_checkbox_backstitch;
};