#include <vector>
#include <iostream>
#include "splashscreen_window.hpp"
#include "x_stitch_editor.hpp"
#include "new_project_window.hpp"
#include "project.hpp"

void SplashScreenWindow::initialise() {
    using namespace nanogui;
    set_layout(new GroupLayout(5, 5, 10, 0));

    Button *button = new Button(this, "Create new project");
    button->set_callback([this](){
        XStitchEditorApplication* app = (XStitchEditorApplication*) m_parent;
        app->switch_application_state(ApplicationStates::CREATE_PROJECT);
    });
    button->set_font_size(30);

    button = new Button(this, "Create new project from an image");
    button->set_callback([this](){
        XStitchEditorApplication* app = (XStitchEditorApplication*) m_parent;
        app->switch_application_state(ApplicationStates::CREATE_DITHERED_PROJECT);
    });
    button->set_font_size(30);

    button = new Button(this, "Load a project");
    button->set_callback([this](){
        XStitchEditorApplication* app = (XStitchEditorApplication*) m_parent;
        app->open_project();
    });
    button->set_font_size(30);

    center();
}