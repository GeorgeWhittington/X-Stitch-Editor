#include "mouse_position_window.hpp"
#include <nanogui/nanogui.h>
#include <fmt/core.h>

using namespace nanogui;
using nanogui::Vector2i;

void MousePositionWindow::initialise() {
    set_layout(new GroupLayout(5, 5, 0, 0));
    _mouse_location_label = new Label(this, "");
    _mouse_location_label_2 = new Label(this, "");
    set_visible(false);
};

void MousePositionWindow::set_captions(int stitch_x, int stitch_y, int offset, Thread *thread) {
    _mouse_location_label->set_caption(fmt::format("stitch selected: {}, {}", stitch_x, stitch_y));
    if (thread != nullptr) {
        _mouse_location_label_2->set_caption(fmt::format("thread: {} {}", thread->company, thread->number));
        set_position(Vector2i(0, offset - 42));
    } else {
        _mouse_location_label_2->set_caption("");
        set_position(Vector2i(0, offset - 26));
    }
    set_visible(true);
};