#include "mouse_position_window.hpp"
#include "x_stitch_editor.hpp"
#include <nanogui/nanogui.h>
#include <fmt/core.h>

using namespace nanogui;
using nanogui::Vector2i;

void MousePositionWindow::initialise() {
    set_layout(new GroupLayout(5, 5, 0, 0));
    _mouse_location_label = new Label(this, "");
    _mouse_location_label_2 = new Label(this, "");
};

void MousePositionWindow::set_captions(int stitch_x, int stitch_y, Thread *thread) {
    int screen_height = _app->framebuffer_size()[1] / _app->pixel_ratio();

    _mouse_location_label->set_caption(fmt::format("stitch selected: {}, {}", stitch_x, stitch_y));
    if (thread != nullptr) {
        _mouse_location_label_2->set_caption(fmt::format("thread: {}", thread->full_name(thread->default_position())));
        set_position(Vector2i(0, screen_height - 42));
    } else {
        _mouse_location_label_2->set_caption("");
        set_position(Vector2i(0, screen_height - 26));
    }
    set_visible(true);
};