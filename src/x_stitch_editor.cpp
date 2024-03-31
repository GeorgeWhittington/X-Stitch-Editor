#include <functional>
#include <iostream>

#include <nanogui/nanogui.h>

#include "x_stitch_editor.hpp"
#include "tool_window.hpp"
#include "mouse_position_window.hpp"

using nanogui::Vector2i;

XStitchEditorApplication::XStitchEditorApplication() : nanogui::Screen(Vector2i(1024, 748), "X Stitch Editor", true) {
    using namespace nanogui;

    load_all_threads();

    LargeIconTheme *theme = new LargeIconTheme(nvg_context());
    set_theme(theme);

    tool_window = new ToolWindow(this);
    tool_window->initialise();

    mouse_position_window = new MousePositionWindow(this);
    mouse_position_window->initialise();
    // create splashscreen window

    // create new project window

    perform_layout();
};

void XStitchEditorApplication::load_all_threads() {
    std::map<std::string, Thread*> *dmc_threads = new std::map<std::string, Thread*>;
    load_manufacturer("../assets/manufacturers/DMC.xml", dmc_threads);
    threads["DMC"] = dmc_threads;
};