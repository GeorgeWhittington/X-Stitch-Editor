#include <iostream>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/slider.h>

using namespace nanogui;

class XStitchEditorApplication : public Screen {
public:
    XStitchEditorApplication() : Screen(Vector2i(1024, 768), "X Stitch Editor") {
        inc_ref();

        Window *window = new Window(this, "");
        window->set_position(Vector2i(0, 0));
        window->set_layout(new GroupLayout(5, 5, 0, 0));

        Label *l = new Label(window, "MODULATION", "sans-bold");
        l->set_font_size(10);
        Slider *slider = new Slider(window);
        slider->set_value(0.5f);
        float modulation = 5.0f;
        slider->set_callback([&modulation](float value) { modulation = value * 10.0f; });

        perform_layout();
    }
};

int main(int, char **) {
    nanogui::init();

    {
        ref<XStitchEditorApplication> app = new XStitchEditorApplication();
        app->dec_ref();
        app->draw_all();
        app->set_visible(true);
        nanogui::mainloop(1 / 60.f * 1000);
    }

    nanogui::shutdown();
    exit(EXIT_SUCCESS);
}