#include <iostream>
#include <map>
#include <string>

#include <nanogui/nanogui.h>
#include <GLFW/glfw3.h>
#include <tinyxml2.h>

#include "x_stitch_editor.hpp"
#include "constants.hpp"

#if defined(_WIN32)
#  if defined(APIENTRY)
#    undef APIENTRY
#  endif
#  include <windows.h>
#endif

using nanogui::ref;

// thread registry
// threads = map of manufacturers to maps of thread no to thread struct

int main(int /* argc */, char ** /* argv */) {
    std::map<std::string, std::map<std::string, Thread*>> threads;

    std::map<std::string, Thread*> dmc_threads;

    tinyxml2::XMLDocument dmc_doc;
    dmc_doc.LoadFile("../assets/manufacturers/DMC.xml");

    tinyxml2::XMLElement *manufacturer = dmc_doc.FirstChildElement("manufacturer");

    // TODO: error handling
    const char *manufacturer_name = "Unknown";
    manufacturer->QueryStringAttribute("name", &manufacturer_name);

    tinyxml2::XMLElement *thread = manufacturer->FirstChildElement("thread");
    const char *thread_number;
    const char *thread_description;
    int thread_r;
    int thread_g;
    int thread_b;

    while (true) {
        // TODO: error handling
        thread->QueryStringAttribute("id", &thread_number);
        thread->QueryStringAttribute("description", &thread_description);
        thread->QueryIntAttribute("red", &thread_r);
        thread->QueryIntAttribute("green", &thread_g);
        thread->QueryIntAttribute("blue", &thread_b);

        Thread *thread_struct = new Thread;
        *thread_struct = {manufacturer_name, thread_number, thread_description, thread_r, thread_g, thread_b};

        dmc_threads[thread_number] = thread_struct;

        thread = thread->NextSiblingElement("thread");
        if (thread == nullptr)
            break;
    }

    threads[manufacturer_name] = dmc_threads;
    std::cout << (threads["DMC"]["Ecru"])->description << std::endl;

    try {
        nanogui::init();

        /* scoped variables */ {
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