#include "main_menu_window.hpp"
#include "x_stitch_editor.hpp"
#include "tool_window.hpp"
#include <GLFW/glfw3.h>

using nanogui::Vector2i;

void MainMenuWindow::initialise() {
    position_top_left();

    set_layout(new nanogui::BoxLayout(
        nanogui::Orientation::Horizontal,
        nanogui::Alignment::Fill, 0, 0));

    nanogui::PopupButton *button = new nanogui::PopupButton(this, "", FA_BARS);
    button->set_chevron_icon(0);
    button->set_side(nanogui::Popup::Side::Left);
    nanogui::Popup *menu = button->popup();
    menu->set_layout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical,
        nanogui::Alignment::Fill, 5, 5));
    menu->set_anchor_offset(15);

    button = new nanogui::PopupButton(menu, "File");
    button->set_chevron_icon(0);
    button->set_side(nanogui::Popup::Side::Left);
    nanogui::Popup *sub_menu = button->popup();
    sub_menu->set_layout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical,
        nanogui::Alignment::Fill, 5, 5));
    sub_menu->set_anchor_offset(20);

    nanogui::Button *menu_button = new nanogui::Button(sub_menu, "New Project");
    menu_button->set_callback([this](){ new_project(); });
    menu_button = new nanogui::Button(sub_menu, "New Project From Image");
    menu_button->set_callback([this](){ new_project_from_image(); });
    menu_button = new nanogui::Button(sub_menu, "Open Project");
    menu_button->set_callback([this](){ open_project(); });
    menu_button = new nanogui::Button(sub_menu, "Close Project");
    menu_button->set_callback([this](){ close_project(); });
    menu_button = new nanogui::Button(sub_menu, "Save");
    menu_button->set_callback([this](){ save(); });
    menu_button = new nanogui::Button(sub_menu, "Save As");
    menu_button->set_callback([this](){ save_as(); });
    menu_button = new nanogui::Button(sub_menu, "Export to PDF");
    menu_button->set_callback([this](){ export_to_pdf(); });

    button = new nanogui::PopupButton(menu, "Edit");
    button->set_chevron_icon(0);
    button->set_side(nanogui::Popup::Side::Left);
    sub_menu = button->popup();
    sub_menu->set_layout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical,
        nanogui::Alignment::Fill, 5, 5));

    menu_button = new nanogui::Button(sub_menu, "Undo");
    menu_button = new nanogui::Button(sub_menu, "Redo");

    button = new nanogui::PopupButton(menu, "View");
    button->set_chevron_icon(0);
    button->set_side(nanogui::Popup::Side::Left);
    sub_menu = button->popup();
    sub_menu->set_layout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical,
        nanogui::Alignment::Fill, 5, 5));

    menu_button = new nanogui::Button(sub_menu, "Show Tools");
    menu_button->set_callback([this](){ toggle_tools(); });
    menu_button = new nanogui::Button(sub_menu, "Show Minimap");
    menu_button->set_callback([this](){ toggle_minimap(); });

    menu_button = new nanogui::Button(menu, "Help");
}

void MainMenuWindow::position_top_left() {
    XStitchEditorApplication *app = (XStitchEditorApplication*)m_parent;
    int screen_width = app->framebuffer_size()[0] / app->pixel_ratio();
    set_position(Vector2i(screen_width - 60, 10));
}

void MainMenuWindow::new_project() {
    // TODO
}

void MainMenuWindow::new_project_from_image() {
    // TODO
}

void MainMenuWindow::open_project() {
    // TODO
}

void MainMenuWindow::close_project() {
    // TODO: check with modal if user wants to save

    _app->switch_project(nullptr);
    _app->switch_application_state(ApplicationStates::LAUNCH);
}

void MainMenuWindow::save() {
    // TODO
}

void MainMenuWindow::save_as() {
    // TODO
}

void MainMenuWindow::export_to_pdf() {
    // TODO
}

void MainMenuWindow::toggle_tools() {
    _app->tool_window->set_visible(!_app->tool_window->visible());
}

void MainMenuWindow::toggle_minimap() {
    // TODO
}