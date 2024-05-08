#include "dithering_window.hpp"
#include "x_stitch_editor.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"
#include "dithering.hpp"
#include "constants.hpp"
#include <iostream>

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

    new Label(form_widget, "Project title:");
    _title_entry = new TextBox(form_widget, "");
    _title_entry->set_editable(true);
    _title_entry->set_alignment(TextBox::Alignment::Left);

    // TODO: option to select how to resize the selected image.
    // Would be nice to do this smartly:
    // * separate width and height inputs
    // * link icon button that lets you lock asp ratio, and then edits to one axis influence the other
    // * probably lay them out in a specific widget, one under the other with the asp ratio button in the right column

    new Label(form_widget, "Canvas background colour:");
    _color_picker = new ColorPicker(form_widget, CANVAS_DEFAULT_COLOR);

    new Label(form_widget, "Algorithm:");
    Widget *algorithm_widget = new Widget(form_widget);
    algorithm_widget->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 5));
    _algorithm_combobox = new ComboBox(algorithm_widget, std::vector{std::string("Floyd-Steinburg"), std::string("Bayer"), std::string("Quantise")});
    _algorithm_combobox->set_callback([this](int index_selected) {
        _app->perform_layout();
    });
    _algorithm_combobox->set_fixed_width(200);
    Button *algorithm_info_button = new Button(algorithm_widget, "", FA_INFO);
    algorithm_info_button->set_tooltip("Different algorithms are more suited for different applications. For images that mostly contain flat colours and no gradients Quantising is suited. Floyd-Steinburg is good for conversion of photographs or other 'busy' images. Bayer has a distinct cross hatch style which is good for creating a retro effect.");
    algorithm_info_button->set_enabled(false);

    new Label(form_widget, "Threads available:");
    Widget *palette_widget = new Widget(form_widget);
    palette_widget->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 5));
    for (const auto & [manufacturer_name, threads_map] : _app->_threads)
        _palette_checkboxes.push_back(new CheckBox(palette_widget, manufacturer_name));
    CheckBox *first = nullptr;
    first = _palette_checkboxes.at(0);
    if (first != nullptr)
        first->set_checked(true);

    new Label(form_widget, "Enable setting a maximum NO threads:");
    _enable_max_threads_checkbox = new CheckBox(form_widget, "");
    _enable_max_threads_checkbox->set_callback([this](bool checked) {
        _max_threads_label->set_visible(checked);
        _max_threads_intbox->set_visible(checked);
        _app->perform_layout();
    });

    // TODO: repace with slider that's range is from 0..total colours in selected palettes
    // (this will obviously need adjusting once colour blending is implemented, not sure how)
    _max_threads_label = new Label(form_widget, "Maximum NO threads:");
    _max_threads_label->set_visible(false);
    _max_threads_intbox = new IntBox<int>(form_widget, 50);
    _max_threads_intbox->set_min_value(1);
    _max_threads_intbox->set_editable(true);
    _max_threads_intbox->set_spinnable(true);
    _max_threads_intbox->set_default_value("50");
    _max_threads_intbox->set_visible(false);
    _max_threads_intbox->set_alignment(TextBox::Alignment::Left);

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
    _color_picker->set_color(CANVAS_DEFAULT_COLOR);
    // TODO: minimise colour_picker if it's left open (do same for create new project form)
    _algorithm_combobox->set_selected_index(0);

    nanogui::CheckBox *cb;
    for (int i = 0; i < _palette_checkboxes.size(); i++) {
        cb = _palette_checkboxes[i];
        cb->set_checked(i == 0); // only check first item
    }

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
        std::cout << "Error loading image: " << stbi_failure_reason() << std::endl;
        // TODO: display error to user
        _app->switch_application_state(ApplicationStates::LAUNCH);
    }
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
        project = new Project(_title_entry->value(), _width, _height, _color_picker->color());
    } catch (const std::invalid_argument& err) {
        _errors->set_caption(err.what());
        _errors->set_visible(true);
        _app->perform_layout();
        return;
    }

    // TODO: once resizing is added, the image, width and height passed in will need to
    // depend on whether the user alters the dimensions of the image. That will happen just
    // before this step here
    int selected_algorithm = _algorithm_combobox->selected_index();
    if (selected_algorithm == DitheringAlgorithms::FLOYD_STEINBURG) {
        FloydSteinburg floyd_steinburg(&palette, max_threads);
        floyd_steinburg.dither(_image, _width, _height, project);
    } else if (selected_algorithm == DitheringAlgorithms::BAYER) {
        Bayer bayer(&palette, max_threads);
        bayer.dither(_image, _width, _height, project);
    } else if (selected_algorithm == DitheringAlgorithms::QUANTISE) {
        NoDither no_dither(&palette, max_threads);
        no_dither.dither(_image, _width, _height, project);
    } else {
        delete project;
        _errors->set_visible(true);
        _errors->set_caption("The algorithm selected is not recognised, please try another");
        _app->perform_layout();
        return;
    }

    _app->switch_project(project);
    _app->switch_application_state(ApplicationStates::PROJECT_OPEN);
    stbi_image_free(_image);
    _image = nullptr;
    _width = 0;
    _height = 0;
}