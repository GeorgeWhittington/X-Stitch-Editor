#pragma once
#include <nanogui/nanogui.h>

class XStitchEditorApplication;

class MainMenuWindow : public nanogui::Window {
public:
    MainMenuWindow(nanogui::Widget *parent) : _app((XStitchEditorApplication*)parent), nanogui::Window(parent, "") {};
    void initialise();
    void position_top_left();
    void update_tool_toggle_icon();
    void update_minimap_toggle_icon();
    void close_all_submenus();

    nanogui::PopupButton *_menu_button;
    nanogui::PopupButton *_file_button;
    nanogui::PopupButton *_edit_button;
    nanogui::PopupButton *_view_button;

    nanogui::Button *_show_tools_button;
    nanogui::Button *_show_minimap_button;

private:
    XStitchEditorApplication *_app;

    void new_project();
    void new_project_from_image();
    void open_project();
    void close_project();
    bool save();
    bool save_as();
    void export_to_pdf();

    void toggle_tools();
    void toggle_minimap();
};