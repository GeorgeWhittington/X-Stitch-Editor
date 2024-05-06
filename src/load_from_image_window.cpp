#include "load_from_image_window.hpp"
#include "constants.hpp"
#include "x_stitch_editor.hpp"

std::vector<std::pair<std::string, std::string>> permitted_image_files = {{"png", ""}, {"jpg", ""}, {"jpeg", ""}};

void LoadFromImageWindow::initialise() {
    using namespace nanogui;
    set_layout(new GroupLayout(5, 5, 10, 0));

    Button *button = new Button(this, "Reduce Colours Smartly (Dithering mode)");
    button->set_callback([this](){
        std::string path = nanogui::file_dialog(permitted_image_files, false);
        if (path == "")
            return;

        try {
            auto image = new cimg_library::CImg<unsigned int>(path.c_str());
            // TODO: _app->dithering_window->set_file(path);
            _app->switch_application_state(ApplicationStates::CREATE_DITHERED_PROJECT);
        } catch (const cimg_library::CImgIOException&) {
            // TODO: display error
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