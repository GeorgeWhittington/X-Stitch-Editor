#include <iostream>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/slider.h>

using namespace nanogui;

int main() {
    nanogui::init();

    nanogui::Screen app(Vector2i(1024, 768), "NanoGUI Test");

    nanogui::Window window{&app, ""};
    window.set_position(Vector2i(15, 15));
    window.set_layout(new nanogui::GroupLayout(5, 5, 0, 0));

    nanogui::Label *l = new nanogui::Label(&window, "MODULATION", "sans-bold");
    l->set_font_size(10);
    nanogui::Slider *slider = new nanogui::Slider(&window);
    slider->set_value(0.5f);
    float modulation = 5.0f;
    slider->set_callback([&modulation](float value) { modulation = value * 10.0f; });

    app.perform_layout();

    app.draw_all();
    app.set_visible(true);

    nanogui::mainloop(1 / 60.f * 1000);

    nanogui::shutdown();
    exit(EXIT_SUCCESS);
}