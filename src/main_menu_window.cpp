#include <vector>
#include <iostream>

#include <GLFW/glfw3.h>

#include "main_menu_window.hpp"
#include "x_stitch_editor.hpp"
#include "tool_window.hpp"
#include "pdf_window.hpp"
#include "project.hpp"
#include "dithering_window.hpp"

using nanogui::Vector2i;

extern std::vector<std::pair<std::string, std::string>> permitted_files;

void MainMenuWindow::initialise() {
    position_top_left();

    set_layout(new nanogui::BoxLayout(
        nanogui::Orientation::Horizontal,
        nanogui::Alignment::Fill, 0, 0));

    _menu_button = new nanogui::PopupButton(this, "", FA_BARS);
    _menu_button->set_chevron_icon(0);
    _menu_button->set_side(nanogui::Popup::Side::Left);
    _menu_button->set_callback([this](){ close_all_submenus(); });
    nanogui::Popup *menu = _menu_button->popup();
    menu->set_layout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical,
        nanogui::Alignment::Fill, 5, 5));
    menu->set_anchor_offset(15);

    _file_button = new nanogui::PopupButton(menu, "File");
    _file_button->set_chevron_icon(0);
    _file_button->set_side(nanogui::Popup::Side::Left);
    nanogui::Popup *sub_menu = _file_button->popup();
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

    _edit_button = new nanogui::PopupButton(menu, "Edit");
    _edit_button->set_chevron_icon(0);
    _edit_button->set_side(nanogui::Popup::Side::Left);
    sub_menu = _edit_button->popup();
    sub_menu->set_layout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical,
        nanogui::Alignment::Fill, 5, 5));

    menu_button = new nanogui::Button(sub_menu, "Undo", FA_UNDO);
    menu_button = new nanogui::Button(sub_menu, "Redo", FA_REDO);

    _view_button = new nanogui::PopupButton(menu, "View");
    _view_button->set_chevron_icon(0);
    _view_button->set_side(nanogui::Popup::Side::Left);
    sub_menu = _view_button->popup();
    sub_menu->set_layout(new nanogui::BoxLayout(
        nanogui::Orientation::Vertical,
        nanogui::Alignment::Fill, 5, 5));

    _show_tools_button = new nanogui::Button(sub_menu, "Show Tools");
    _show_tools_button->set_callback([this](){ toggle_tools(); });
    _show_minimap_button = new nanogui::Button(sub_menu, "Show Minimap");
    _show_minimap_button->set_callback([this](){ toggle_minimap(); });

    menu_button = new nanogui::Button(menu, "Help");
}

void MainMenuWindow::position_top_left() {
    XStitchEditorApplication *app = (XStitchEditorApplication*)m_parent;
    int screen_width = app->framebuffer_size()[0] / app->pixel_ratio();
    set_position(Vector2i(screen_width - 60, 10));
}

void MainMenuWindow::close_all_submenus() {
    _file_button->set_pushed(false);
    _edit_button->set_pushed(false);
    _view_button->set_pushed(false);
}

void MainMenuWindow::close_all_menus() {
    close_all_submenus();
    _menu_button->set_pushed(false);
}

void MainMenuWindow::new_project() {
    auto dlg = new nanogui::MessageDialog(_app, nanogui::MessageDialog::Type::Question, "", "Do you want to save your current project first?", "Yes", "No", true);
    dlg->set_callback([this](int result) {
        if (!result) {
            bool saved = save();
            if (!saved) {
                close_all_menus();
                return;
            }
        }

        _app->switch_project(nullptr);
        _app->switch_application_state(ApplicationStates::CREATE_PROJECT);
    });
}

void MainMenuWindow::new_project_from_image() {
    auto dlg = new nanogui::MessageDialog(_app, nanogui::MessageDialog::Type::Question, "", "Do you want to save your current project first?", "Yes", "No", true);
    dlg->set_callback([this](int result) {
        if (!result) {
            bool saved = save();
            if (!saved) {
                close_all_menus();
                return;
            }
        }

        _app->switch_project(nullptr);
        _app->switch_application_state(ApplicationStates::CREATE_PROJECT_FROM_IMAGE);
        _app->dithering_window->select_image();
    });
}

void MainMenuWindow::open_project() {
    auto dlg = new nanogui::MessageDialog(_app, nanogui::MessageDialog::Type::Question, "", "Do you want to save your current project first?", "Yes", "No", true);
    dlg->set_callback([this](int result) {
        if (!result) {
            bool saved = save();
            if (!saved) {
                close_all_menus();
                return;
            }
        }

        _app->open_project();
    });
}

void MainMenuWindow::close_project() {
    auto dlg = new nanogui::MessageDialog(_app, nanogui::MessageDialog::Type::Question, "", "Do you want to save your project before closing?", "Yes", "No", true);
    dlg->set_callback([this](int result) {
        if (!result) {
            bool saved = save();
            if (!saved) {
                close_all_menus();
                return;
            }
        }

        _app->switch_project(nullptr);
        _app->switch_application_state(ApplicationStates::LAUNCH);
    });
}

bool MainMenuWindow::save() {
    if (_app->_project->file_path == "") {
        bool saved = save_as();
        return saved;
    } else {
        _app->_project->save(_app->_project->file_path.c_str(), _app);
        close_all_menus();
        return true;
    }
}

bool MainMenuWindow::save_as() {
    std::string path = nanogui::file_dialog(permitted_files, true);
    if (path != "") {
        _app->_project->save(path.c_str(), _app);
        close_all_menus();
        return true;
    }
    return false;
}

void MainMenuWindow::export_to_pdf() {
    _app->pdf_window->set_visible(true);
}

void MainMenuWindow::update_tool_toggle_icon() {
    int icon = _app->tool_window->visible() ? FA_CHECK : 0;
    _show_tools_button->set_icon(icon);
}

void MainMenuWindow::toggle_tools() {
    _app->tool_window->set_visible(!_app->tool_window->visible());
    update_tool_toggle_icon();
}

void MainMenuWindow::update_minimap_toggle_icon() {
    // TODO
}

void MainMenuWindow::toggle_minimap() {
    // TODO
}