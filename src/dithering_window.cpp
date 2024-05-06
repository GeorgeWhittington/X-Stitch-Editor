#include "dithering_window.hpp"
#include "x_stitch_editor.hpp"

void DitheringWindow::initialise() {
    using namespace nanogui;

    set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 15, 15));

    _errors = new Label(this, "");
    _errors->set_color(Color(204, 0, 0, 255));
    _errors->set_visible(false);

    // TODO: widget that displays the selected image and shows some info about it
    // if there's time, displaying a preview of what the currently selected dithering settings
    // will produce would be cool
    // could make this less intensive with a button at the bottom called preview?

    // TODO: set fixed width for each column so that combobox changes
    // don't make the whole window resize, it looks bad
    Widget *form_widget = new Widget(this);
    GridLayout *layout = new GridLayout(Orientation::Horizontal, 2, Alignment::Minimum, 0, 5);
    layout->set_col_alignment(Alignment::Fill);
    form_widget->set_layout(layout);

    new Label(form_widget, "Algorithm:");
    _algorithm_combobox = new ComboBox(form_widget, std::vector{std::string("Floyd-Steinburg"), std::string("Bayer")});
    _algorithm_combobox->set_callback([this](int index_selected) {
        _app->perform_layout();
    });

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

    _max_threads_label = new Label(form_widget, "Maximum NO threads:");
    _max_threads_label->set_visible(false);
    _max_threads_intbox = new IntBox<int>(form_widget);
    // TODO: set all the stuff for the intbox to function correctly
    _max_threads_intbox->set_visible(false);

    Button *button = new Button(this, "Create pattern");
    button->set_callback([this]() {
        // TODO: do dithering, switch application_state
    });

    // TODO: option to select how to resize the selected image.
    // Would be nice to do this smartly:
    // * separate width and height inputs
    // * link icon button that lets you lock asp ratio, and then edits to one axis influence the other

    center();
}

void DitheringWindow::reset_form() {
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
}