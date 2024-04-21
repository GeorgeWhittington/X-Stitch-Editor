#include "new_project_window.hpp"
#include <nanogui/nanogui.h>
#include <vector>
#include "x_stitch_editor.hpp"
#include "tool_window.hpp"
#include "project.hpp"

const nanogui::Color CANVAS_DEFAULT_COLOR = nanogui::Color(255, 255, 255, 255);

void NewProjectWindow::initialise() {
    using namespace nanogui;

    set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 15, 15));

    _errors = new Label(this, "");
    _errors->set_color(Color(204, 0, 0, 255));
    _errors->set_visible(false);

    Widget *form_widget = new Widget(this);
    GridLayout *layout = new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 0, 5);
    layout->set_col_alignment(std::vector{Alignment::Fill, Alignment::Fill});
    form_widget->set_layout(layout);

    Label *label = new Label(form_widget, "Project Title:");
    _title_entry = new TextBox(form_widget, "");
    _title_entry->set_editable(true);
    _title_entry->set_alignment(TextBox::Alignment::Left);

    label = new Label(form_widget, "Canvas Width:");
    _width_entry = new IntBox<int>(form_widget, 100);

    label = new Label(form_widget, "Canvas Height:");
    _height_entry = new IntBox<int>(form_widget, 100);

    for (auto entry : {_width_entry, _height_entry}) {
        entry->set_default_value("100");
        entry->set_units("stitches");
        entry->set_editable(true);
        entry->set_alignment(TextBox::Alignment::Left);
        entry->set_fixed_width(200);
        entry->set_value_increment(10);
        entry->set_spinnable(true);
        entry->set_min_value(1);
    }

    label = new Label(form_widget, "Canvas Background Colour:");
    _color_picker = new ColorPicker(form_widget, CANVAS_DEFAULT_COLOR);

    Button *confirm_button = new Button(this, "Confirm");
    confirm_button->set_callback([this](){
        _errors->set_visible(false);

        XStitchEditorApplication *app = (XStitchEditorApplication*) m_parent;

        Project *project;

        try {
            project = new Project(
                _title_entry->value(),
                _width_entry->value(),
                _height_entry->value(),
                _color_picker->color()
            );
        } catch (const std::invalid_argument& err) {
            _errors->set_caption(err.what());
            _errors->set_visible(true);
            app->perform_layout();
            return;
        }

        app->switch_project(project);

        app->switch_application_state(ApplicationStates::PROJECT_OPEN);
    });

    center();
}

void NewProjectWindow::reset_form() {
    _errors->set_visible(false);
    _title_entry->set_value("");
    _width_entry->set_value(100);
    _height_entry->set_value(100);
    _color_picker->set_color(CANVAS_DEFAULT_COLOR);
}