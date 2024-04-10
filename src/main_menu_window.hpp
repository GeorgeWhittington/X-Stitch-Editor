#pragma once
#include <nanogui/nanogui.h>

class XStitchEditorApplication;

class MainMenuWindow : public nanogui::Window {
public:
    MainMenuWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();
    void position_top_left();

private:
    XStitchEditorApplication *_app;

    void new_project();
    void new_project_from_image();
    void open_project();
    void close_project();
    void save();
    void save_as();
    void export_to_pdf();

    void toggle_tools();
    void toggle_minimap();
};