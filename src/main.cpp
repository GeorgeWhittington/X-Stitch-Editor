#include <iostream>

#include <GLFW/glfw3.h>
#include <nanogui/nanogui.h>

#include "x_stitch_editor.hpp"

#if defined(_WIN32)
#  if defined(APIENTRY)
#    undef APIENTRY
#  endif
#  include <windows.h>
#endif

int main(int, char **) {
    try {
        nanogui::init();

        {
            nanogui::ref<XStitchEditorApplication> app = new XStitchEditorApplication();
            app->draw_all();
            app->set_visible(true);
            nanogui::mainloop(1 / 60.f * 1000);
        }

        nanogui::shutdown();
    } catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(_WIN32)
            MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            std::cerr << error_msg << std::endl;
        #endif
        return -1;
    }

    return 0;
}