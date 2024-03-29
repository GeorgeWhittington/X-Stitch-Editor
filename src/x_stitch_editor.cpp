#include <functional>

#include <nanogui/nanogui.h>

#include "x_stitch_editor.hpp"
#include "tool_window.hpp"

using nanogui::Vector2i;

XStitchEditorApplication::XStitchEditorApplication() : nanogui::Screen(Vector2i(1024, 748), "X Stitch Editor", true) {
    using namespace nanogui;

    LargeIconTheme *theme = new LargeIconTheme(nvg_context());
    set_theme(theme);

    tool_window = new ToolWindow(this);
    tool_window->initialise();
    // create mouse pos window
    // create splashscreen window

    // create new project window

    perform_layout();
};