#include "load_from_image_window.hpp"
#include "constants.hpp"
#include "x_stitch_editor.hpp"
#include "dithering_window.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"
#include <iostream>

std::vector<std::pair<std::string, std::string>> permitted_image_files = {{"png", ""}, {"jpg", ""}, {"jpeg", ""}};

void LoadFromImageWindow::initialise() {
    using namespace nanogui;
    set_layout(new GroupLayout(5, 5, 10, 0));

    Button *button = new Button(this, "Reduce Colours Smartly (Dithering mode)");
    button->set_callback([this](){
        std::string path = nanogui::file_dialog(permitted_image_files, false);
        if (path == "")
            return;

        // TODO: the image pointer returned can be later freed with stbi_image_free
        int width, height, no_channels;
        unsigned char *image = stbi_load(path.c_str(), &width, &height, &no_channels, 4);  // requesting 4 channels

        if (image != nullptr) {
            _app->dithering_window->set_input_image(image, width, height);
            _app->switch_application_state(ApplicationStates::CREATE_DITHERED_PROJECT);
        } else {
            // TODO: display error
            // Use stbi_failure_reason() to get actual err msg?
            // Will need to see if the messages with or without STBI_FAILURE_USERMSG
            // are good enough to pass through directly to the user, or if a simple
            // "error loading this image" msg is better. Probably one the former.
        }
    });
    button->set_font_size(30);
    button->set_tooltip("This mode converts images using a dithering algorithm, and is more suited for photographs or images without many areas of flat colour.");

    button = new Button(this, "Reduce Colours Naively (Quantizing mode)");
    button->set_callback([this](){
        std::string path = nanogui::file_dialog(permitted_image_files, false);
        if (path == "")
            return;

        // TODO: _app->quantising_window->set_file(path);
        _app->switch_application_state(ApplicationStates::CREATE_QUANTISED_PROJECT);
    });
    button->set_font_size(30);
    button->set_tooltip("This mode converts images using quantization, picking the closest available thread for each pixel, and is more suited for images with areas of flat colour such as clip art.");

    center();
}