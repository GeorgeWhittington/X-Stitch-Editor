#include "splashscreen_window.hpp"
#include "x_stitch_editor.hpp"
#include "new_project_window.hpp"

void SplashScreenWindow::initialise() {
    using namespace nanogui;
    set_layout(new GroupLayout(5, 5, 10, 0));

    Button *button = new Button(this, "Create new project");
    button->set_callback([this](){
        set_visible(false);
        XStitchEditorApplication* app = (XStitchEditorApplication*) m_parent;
        app->new_project_window->set_visible(true);
        app->perform_layout();
    });
    button->set_font_size(30);

    button = new Button(this, "Create new project from an image");
    button->set_callback([this](){
        // TODO
    });
    button->set_font_size(30);

    button = new Button(this, "Load a project");
    button->set_callback([this](){
        // TODO
    });
    button->set_font_size(30);

    center();
}