#include "dithering_window.hpp"
#include "x_stitch_editor.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#include "dithering.hpp"
#include "constants.hpp"
#include <iostream>
#include <chrono>

using namespace std::chrono;

std::vector<std::pair<std::string, std::string>> permitted_image_files = {{"png", ""}, {"jpg", ""}, {"jpeg", ""}};

void DitheringWindow::initialise() {
    using namespace nanogui;

    set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 15, 15));

    _errors = new Label(this, "");
    _errors->set_color(Color(204, 0, 0, 255));
    _errors->set_visible(false);

    Widget *form_widget = new Widget(this);
    GridLayout *layout = new GridLayout(Orientation::Horizontal, 2, Alignment::Minimum, 0, 5);
    layout->set_col_alignment(Alignment::Fill);
    layout->set_row_alignment(std::vector{Alignment::Middle, Alignment::Minimum});
    form_widget->set_layout(layout);

    // TITLE
    new Label(form_widget, "Project title:");
    _title_entry = new TextBox(form_widget, "");
    _title_entry->set_editable(true);
    _title_entry->set_alignment(TextBox::Alignment::Left);

    // DIMENSIONS
    new Label(form_widget, "Project dimensions:");
    Widget *dimensions_widget = new Widget(form_widget);
    Label *width_label = new Label(dimensions_widget, "Width:");
    _width_intbox = new IntBox(dimensions_widget, 0);
    _width_intbox->set_callback([this](int width) {
        if (!_aspect_ratio_button->pushed())
            return;

        float ratio = (float)_height / (float)_width;
        _height_intbox->set_value(width * ratio);
    });
    Label *height_label = new Label(dimensions_widget, "Height:");
    _height_intbox = new IntBox(dimensions_widget, 0);
    _height_intbox->set_callback([this](int height) {
        if (!_aspect_ratio_button->pushed())
            return;

        float ratio = (float)_width / (float)_height;
        _width_intbox->set_value(height * ratio);
    });
    for (auto intbox : {_width_intbox, _height_intbox}) {
        intbox->set_fixed_width(142);
        intbox->set_units("stitches");
    }

    _aspect_ratio_button = new Button(dimensions_widget, "", FA_LINK);
    _aspect_ratio_button->set_flags(Button::ToggleButton);
    _aspect_ratio_button->set_pushed(true);
    Button *reset_dimensions_button = new Button(dimensions_widget, "", FA_UNDO);
    reset_dimensions_button->set_callback([this]() {
        _width_intbox->set_value(_width);
        _height_intbox->set_value(_height);
    });

    using Anchor = AdvancedGridLayout::Anchor;
    AdvancedGridLayout *dimensions_layout = new AdvancedGridLayout(std::vector{0, 5, 0, 5, 0}, std::vector{0, 5, 0}, 0);
    dimensions_widget->set_layout(dimensions_layout);
    dimensions_layout->set_anchor(width_label, Anchor(0, 0));
    dimensions_layout->set_anchor(_width_intbox, Anchor(2, 0));
    dimensions_layout->set_anchor(height_label, Anchor(0, 2));
    dimensions_layout->set_anchor(_height_intbox, Anchor(2, 2));
    dimensions_layout->set_anchor(_aspect_ratio_button, Anchor(4, 0));
    dimensions_layout->set_anchor(reset_dimensions_button, Anchor(4, 2));

    // TODO: Add control for upscaling algorithm (bicubic, point sample)
    // should only appear if user has selected dimensions greater than the input image
    // (STBIR_FILTER_DEFAULT, STBIR_FILTER_POINT_SAMPLE)

    // CANVAS BACKGROUND COLOUR
    new Label(form_widget, "Canvas background colour:");
    _color_picker = new ColorPicker(form_widget, CANVAS_DEFAULT_COLOR);

    // ALGORITHM
    new Label(form_widget, "Algorithm:");
    Widget *algorithm_widget = new Widget(form_widget);
    algorithm_widget->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 5));
    _algorithm_combobox = new ComboBox(algorithm_widget, std::vector<std::string>{"Floyd-Steinburg", "Bayer", "Quantise"});
    _algorithm_combobox->set_callback([this](int index_selected) {
        _app->perform_layout();
        if (index_selected == DitheringAlgorithms::BAYER) {
            _matrix_size_label->set_visible(true);
            _matrix_size_widget->set_visible(true);
            _app->perform_layout();
        } else if (_matrix_size_label->visible() || _matrix_size_widget->visible()) {
            _matrix_size_label->set_visible(false);
            _matrix_size_widget->set_visible(false);
            _app->perform_layout();
        }
    });
    _algorithm_combobox->set_fixed_width(200);
    Button *algorithm_info_button = new Button(algorithm_widget, "", FA_INFO);
    algorithm_info_button->set_tooltip("Different algorithms are more suited for different applications. For images that mostly contain flat colours and no gradients Quantising is suited. Floyd-Steinburg is good for conversion of photographs or other 'busy' images. Bayer has a distinct cross hatch style which is good for creating a retro effect.");
    algorithm_info_button->set_enabled(false);

    // THRESHOLD MATRIX
    _matrix_size_label = new Label(form_widget, "Matrix Size:");
    _matrix_size_widget = new Widget(form_widget);
    _matrix_size_widget->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 5));
    _matrix_size_combobox = new ComboBox(_matrix_size_widget, std::vector<std::string>{"2", "4", "8", "16"});
    _matrix_size_combobox->set_selected_index(1);
    _matrix_size_combobox->set_callback([this](int index_selected) { _app->perform_layout(); });
    _matrix_size_combobox->set_fixed_width(200);
    Button *matrix_info_button = new Button(_matrix_size_widget, "", FA_INFO);
    matrix_info_button->set_tooltip("This setting controls the size of the threshold matrix used to perform Bayer dithering. As the matrix size increases, the resulting image will become brighter. Banding will be quite noticeable at size 2, and the patterns are most recognisable at size 4.");
    matrix_info_button->set_enabled(false);
    // Have to do this in a strange order to make sure info button is sized correctly
    _app->perform_layout();
    _matrix_size_label->set_visible(false);
    _matrix_size_widget->set_visible(false);

    // PALETTE
    new Label(form_widget, "Threads available:");
    Widget *palette_widget = new Widget(form_widget);
    palette_widget->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 5));
    for (const auto & [manufacturer_name, threads_map] : _app->_threads)
        _palette_checkboxes.push_back(new CheckBox(palette_widget, manufacturer_name));
    CheckBox *first = nullptr;
    first = _palette_checkboxes.at(0);
    if (first != nullptr)
        first->set_checked(true);

    // TODO: Setting to only use greyscale colours from selected palettes
    // (Do this automatically, test the colours, see: https://austinpray.com/2020/05/25/detecting-grayscale-colors.html
    // for if I want to be smart about and not just check R = G = B)

    // ENABLE THREAD BLENDING
    new Label(form_widget, "Enable thread blending:");
    _enable_thread_blending_checkbox = new CheckBox(form_widget, "");

    // ENABLE MAX THREADS
    new Label(form_widget, "Enable setting a maximum NO threads:");
    _enable_max_threads_checkbox = new CheckBox(form_widget, "");
    _enable_max_threads_checkbox->set_callback([this](bool checked) {
        _max_threads_label->set_visible(checked);
        _max_threads_intbox->set_visible(checked);
        _app->perform_layout();
    });

    // MAX THREADS
    // TODO: repace with slider that's range is from 0..total colours in selected palettes
    // (this will obviously need adjusting once colour blending is implemented, not sure how)
    _max_threads_label = new Label(form_widget, "Maximum NO threads:");
    _max_threads_label->set_visible(false);
    _max_threads_intbox = new IntBox<int>(form_widget, 50);
    _max_threads_intbox->set_default_value("50");
    _max_threads_intbox->set_visible(false);

    for (auto intbox : {_width_intbox, _height_intbox, _max_threads_intbox}) {
        intbox->set_editable(true);
        intbox->set_spinnable(true);
        intbox->set_alignment(TextBox::Alignment::Left);
        intbox->set_min_value(1);
    }

    Button *button = new Button(this, "Create pattern");
    button->set_callback([this]() {
        create_pattern();
    });

    // TODO: A second "Preview" button would be a very good feature,
    // just a pop-up that displays the result of the currently
    // selected settings

    center();
}

void DitheringWindow::reset_form() {
    _errors->set_visible(false);
    _title_entry->set_value("");
    _aspect_ratio_button->set_pushed(true);
    _color_picker->set_color(CANVAS_DEFAULT_COLOR);
    _color_picker->set_pushed(false);
    _algorithm_combobox->set_selected_index(0);
    _matrix_size_label->set_visible(false);
    _matrix_size_widget->set_visible(false);
    _matrix_size_combobox->set_selected_index(1);

    nanogui::CheckBox *cb;
    for (int i = 0; i < _palette_checkboxes.size(); i++) {
        cb = _palette_checkboxes[i];
        cb->set_checked(i == 0); // only check first item
    }

    _enable_thread_blending_checkbox->set_checked(false);
    _enable_max_threads_checkbox->set_checked(false);
    _max_threads_label->set_visible(false);
    _max_threads_intbox->set_visible(false);
    _max_threads_intbox->set_value(50);

    if (_image != nullptr) {
        stbi_image_free(_image);
        _image = nullptr;
        _width = 0;
        _height = 0;
    }
}

void DitheringWindow::select_image() {
    std::string path = nanogui::file_dialog(permitted_image_files, false);
    if (path == "") {
        _app->switch_application_state(ApplicationStates::LAUNCH);
        return;
    }

    int no_channels;

    // Freeing just incase, ideally all exit points from the window would clear this first
    if (_image != nullptr) {
        stbi_image_free(_image);
        _image = nullptr;
    }

    _image = stbi_load(path.c_str(), &_width, &_height, &no_channels, 4); // requesting 4 channels

    if (_image == nullptr) {
        new nanogui::MessageDialog(_app, nanogui::MessageDialog::Type::Warning, "Error loading image", stbi_failure_reason());
        _app->switch_application_state(ApplicationStates::LAUNCH);
        return;
    }

    _width_intbox->set_value(_width);
    _width_intbox->set_default_value(std::to_string(_width));
    _height_intbox->set_value(_height);
    _height_intbox->set_default_value(std::to_string(_height));
}

void DitheringWindow::create_pattern() {
    _errors->set_visible(false);
    _app->perform_layout();
    // read and validate user input
    int max_threads = INT_MAX;
    if (_enable_max_threads_checkbox->checked()) {
        max_threads = _max_threads_intbox->value();
        if (max_threads < 1) {
            _errors->set_visible(true);
            _errors->set_caption("The maximum NO threads must be 1 or greater");
            _app->perform_layout();
            return;
        }
    }

    std::vector<Thread*> palette;

    nanogui::CheckBox *cb;
    for (int i = 0; i < _palette_checkboxes.size(); i++) {
        cb = _palette_checkboxes[i];
        if (!cb->checked())
            continue;

        auto manufacturer = _app->_threads.begin();
        std::advance(manufacturer, i);
        auto threads = manufacturer->second;

        for (const auto & [key, thread] : *threads) {
            palette.push_back(thread);
        }
    }

    if (palette.size() < 0) {
        _errors->set_visible(true);
        _errors->set_caption("Please select atleast one thread palette (e.g. DMC, Anchor, etc)");
        _app->perform_layout();
        return;
    }

    Project *project;

    try {
        project = new Project(_title_entry->value(), _width_intbox->value(), _height_intbox->value(), _color_picker->color());
    } catch (const std::invalid_argument& err) {
        _errors->set_caption(err.what());
        _errors->set_visible(true);
        _app->perform_layout();
        return;
    }

    // Resizing image if necessary
    int user_width = _width_intbox->value();
    int user_height = _height_intbox->value();
    if (_width != user_width || _height != user_height) {
        unsigned char *resized_image = nullptr;
        resized_image = stbir_resize_uint8_srgb(_image, _width, _height, 0, NULL, user_width, user_height, 0, stbir_pixel_layout::STBIR_RGBA);

        if (resized_image != nullptr) {
            stbi_image_free(_image);
            _image = resized_image;
            _width = user_width;
            _height = user_height;
        } else {
            _errors->set_visible(true);
            _errors->set_caption("Error resizing image");
            _app->perform_layout();
            return;
        }
    }

    auto start = high_resolution_clock::now();

    int selected_algorithm = _algorithm_combobox->selected_index();
    if (selected_algorithm == DitheringAlgorithms::FLOYD_STEINBURG) {
        FloydSteinburg floyd_steinburg(&palette, max_threads, _enable_thread_blending_checkbox->checked());
        floyd_steinburg.dither(_image, _width, _height, project);
    } else if (selected_algorithm == DitheringAlgorithms::BAYER) {
        int selected_matrix = _matrix_size_combobox->selected_index();
        if (selected_matrix == 0) {
            Bayer<BayerOrders::TWO> bayer(&palette, max_threads, _enable_thread_blending_checkbox->checked());
            bayer.dither(_image, _width, _height, project);
        } else if (selected_matrix == 1) {
            Bayer<BayerOrders::FOUR> bayer(&palette, max_threads, _enable_thread_blending_checkbox->checked());
            bayer.dither(_image, _width, _height, project);
        } else if (selected_matrix == 2) {
            Bayer<BayerOrders::EIGHT> bayer(&palette, max_threads, _enable_thread_blending_checkbox->checked());
            bayer.dither(_image, _width, _height, project);
        } else if (selected_matrix == 3) {
            Bayer<BayerOrders::SIXTEEN> bayer(&palette, max_threads, _enable_thread_blending_checkbox->checked());
            bayer.dither(_image, _width, _height, project);
        } else {
            delete project;
            _errors->set_visible(true);
            _errors->set_caption("The matrix size selected is not recognised, please try another");
            _app->perform_layout();
            return;
        }
    } else if (selected_algorithm == DitheringAlgorithms::QUANTISE) {
        NoDither no_dither(&palette, max_threads, _enable_thread_blending_checkbox->checked());
        no_dither.dither(_image, _width, _height, project);
    } else {
        delete project;
        _errors->set_visible(true);
        _errors->set_caption("The algorithm selected is not recognised, please try another");
        _app->perform_layout();
        return;
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    std::cout << "Time elapsed dithering: " << duration.count() << "ms" << std::endl;

    _app->switch_project(project);
    _app->switch_application_state(ApplicationStates::PROJECT_OPEN);
    stbi_image_free(_image);
    _image = nullptr;
    _width = 0;
    _height = 0;
}