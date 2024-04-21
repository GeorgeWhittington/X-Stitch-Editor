#include "pdf_window.hpp"
#include "x_stitch_editor.hpp"
#include "main_menu_window.hpp"
#include "pdf_creation.hpp"
#include <vector>

void checkbox_callback(bool checked, nanogui::CheckBox *this_checkbox, nanogui::CheckBox *that_checkbox) {
    if (!checked && that_checkbox->checked() != true) {
        this_checkbox->set_checked(true);
    }
    if (checked && that_checkbox->checked() == true) {
        that_checkbox->set_checked(false);
    }
}

void PDFWindow::initialise() {
    using namespace nanogui;

    set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 15, 15));

    _errors = new Label(this, "");
    _errors->set_color(Color(204, 0, 0, 255));
    _errors->set_visible(false);

    Widget *form_widget = new Widget(this);
    GridLayout *layout = new GridLayout(Orientation::Horizontal, 2, Alignment::Minimum, 0, 5);
    layout->set_col_alignment(std::vector{Alignment::Fill, Alignment::Fill});
    form_widget->set_layout(layout);

    new Label(form_widget, "Render pattern in:");
    Widget *checkboxes_widget = new Widget(form_widget);
    checkboxes_widget->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 5));
    _checkbox_bw = new CheckBox(
        checkboxes_widget, "Black and White",
        [this](bool checked){ checkbox_callback(checked, _checkbox_bw, _checkbox_colour); }
    );
    _checkbox_colour = new CheckBox(
        checkboxes_widget, "Colour",
        [this](bool checked){ checkbox_callback(checked, _checkbox_colour, _checkbox_bw); }
    );
    _checkbox_colour->set_checked(true);

    new Label(form_widget, "Author:");
    _author_textbox = new TextBox(form_widget, "");
    _author_textbox->set_editable(true);
    _author_textbox->set_alignment(TextBox::Alignment::Left);
    _author_textbox->set_fixed_width(200);
    // _author_textbox->set_

    new Label(form_widget, "Generate Separate Backstitch Chart:");
    _checkbox_backstitch = new CheckBox(form_widget, "");

    Button *create_pdf_button = new Button(this, "Create PDF");
    create_pdf_button->set_callback([this](){ save_pdf(); });

    center();
}

void PDFWindow::clear() {
    _errors->set_visible(false);
    _checkbox_colour->set_checked(true);
    _checkbox_bw->set_checked(false);
    _author_textbox->set_value("");
    _checkbox_backstitch->set_checked(false);
}

void PDFWindow::save_pdf() {
    // TODO: extract and verify input
    if (_checkbox_bw->checked() && _checkbox_colour->checked()) {
        _checkbox_colour->set_checked(true);
        _checkbox_bw->set_checked(false);
        return;
    }

    if (_author_textbox->value() == "") {
        _errors->set_caption("Author cannot be empty!");
        _errors->set_visible(true);
        return;
    }

    if (_author_textbox->value().size() > 50) {
        _errors->set_caption("Author cannot be longer than 50 characters!");
        _errors->set_visible(true);
        return;
    }

    PDFSettings settings{
        _checkbox_colour->checked(),
        _checkbox_backstitch->checked() && _app->_project->backstitches.size() != 0,
        _author_textbox->value()
    };

    // hide and clear to default values
    set_visible(false);
    clear();

    // Pass in settings from wizard here
    PDFWizard pdf_wizard(_app->_project, &settings);

    // Show save dialog, then create and save pdf if a path is returned
    std::vector<std::pair<std::string, std::string>> permitted_pdf_files = {{"pdf", "Portable Document Format"}, {"PDF", "Portable Document Format"}};
    std::string path = nanogui::file_dialog(permitted_pdf_files, true);
    if (path != "")
        pdf_wizard.create_and_save_pdf(path);

    _app->main_menu_window->close_all_menus();
}