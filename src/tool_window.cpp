#include <nanogui/nanogui.h>
#include <iostream>

#include "tool_window.hpp"
#include "x_stitch_editor.hpp"

#define MAKE_TOOLBUTTON_CALLBACK(tool, cursor) [&] {                        \
        XStitchEditorApplication* a = (XStitchEditorApplication*) m_parent; \
        a->_selected_tool = tool;                                           \
        m_parent->set_cursor(cursor);                                       \
    }                                                                       \

using namespace nanogui;

void PaletteButton::palettebutton_callback() {
    _app->_selected_thread = _thread;
    _app->tool_window->update_selected_thread_widget();
};

void ToolWindow::initialise() {
    set_position(Vector2i(0, 0));
    set_layout(new GroupLayout(5, 5, 10, 0));

    // Drawing Tools
    Label *label = new Label(this, "Tools", "sans-bold");

    Widget *tool_wrapper = new Widget(this);
    tool_wrapper->set_layout(new BoxLayout(
        Orientation::Horizontal, Alignment::Fill, 0, 0));

    Widget *tools = new Widget(tool_wrapper);
    GridLayout *tool_layout = new GridLayout(
        Orientation::Horizontal, 5, Alignment::Fill, 0, 0);
    tool_layout->set_spacing(0, 5);
    tools->set_layout(tool_layout);

    auto move_callback = MAKE_TOOLBUTTON_CALLBACK(ToolOptions::MOVE, Cursor::Hand);

    ToolButton *toolbutton = new ToolButton(tools, FA_HAND_PAPER);
    toolbutton->set_tooltip("Move");
    toolbutton->set_callback(move_callback);
    toolbutton->set_pushed(true);
    move_callback();

    toolbutton = new ToolButton(tools, FA_PENCIL_ALT);
    toolbutton->set_tooltip("Single Stitch");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::SINGLE_STITCH, Cursor::Arrow));

    toolbutton = new ToolButton(tools, FA_PEN_NIB);
    toolbutton->set_tooltip("Back Stitch");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::BACK_STITCH, Cursor::Crosshair));

    toolbutton = new ToolButton(tools, FA_ERASER);
    toolbutton->set_tooltip("Erase");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::ERASE, Cursor::Arrow));

    toolbutton = new ToolButton(tools, FA_FILL_DRIP);
    toolbutton->set_tooltip("Fill Area");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::FILL, Cursor::Arrow));

    toolbutton = new ToolButton(tools, FA_SEARCH_PLUS);
    toolbutton->set_tooltip("Zoom In");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::ZOOM_IN, Cursor::VResize));

    toolbutton = new ToolButton(tools, FA_SEARCH_MINUS);
    toolbutton->set_tooltip("Zoom Out");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::ZOOM_OUT, Cursor::VResize));

    // Palette
    label = new Label(this, "Selected Thread", "sans-bold");

    Widget *selected_thread_widget = new Widget(this);
    selected_thread_widget->set_layout(new BoxLayout(
        Orientation::Horizontal, Alignment::Maximum, 0, 5));

    _selected_thread_button = new DisabledButton(selected_thread_widget, "");
    _selected_thread_label = new Label(selected_thread_widget, "");
    update_selected_thread_widget();

    label = new Label(this, "Palette", "sans-bold");

    set_palette();

    // TODO: switch/edit palette button that launches a popup to do so?

    set_visible(false);
};

void ToolWindow::set_palette() {
    if (palette_container != nullptr) {
        delete palette_container;
    }

    VScrollPanel *palette_container = new VScrollPanel(this);
    palette_container->set_fixed_height(470);

    Widget *palette_widget = new Widget(palette_container);
    GridLayout *palette_layout = new GridLayout(
        Orientation::Horizontal, 3, Alignment::Maximum, 0, 5);
    palette_layout->set_col_alignment({ Alignment::Minimum, Alignment::Minimum, Alignment::Maximum });
    palette_widget->set_layout(palette_layout);

    Label *label = new Label(palette_widget, "Company", "sans");
    label = new Label(palette_widget, "Code", "sans");
    label = new Label(palette_widget, "");

    // TODO: take parameter which determines which threads are added here
    auto threads = ((XStitchEditorApplication*)m_parent)->threads;
    auto *DMC_threads = threads["DMC"];

    for (const auto& manufacturers_threads : *DMC_threads) {
        Thread *t = manufacturers_threads.second;

        label = new Label(palette_widget, t->company, "sans");
        label = new Label(palette_widget, t->number, "sans");

        PaletteButton *button = new PaletteButton(palette_widget, "  ");
        button->set_thread(t);
        button->set_app(m_parent);
        button->set_callback();
        button->set_background_color(t->color());
        button->set_tooltip(t->description);
    }
};

void ToolWindow::update_selected_thread_widget() {
    Thread *t = ((XStitchEditorApplication*)m_parent)->_selected_thread;
    if (t == nullptr) {
        _selected_thread_button->set_icon(FA_BAN);
        _selected_thread_label->set_caption("");
    } else {
        _selected_thread_button->set_icon(0);
        std::string thread_number = t->number;
        _selected_thread_label->set_caption(thread_number);
        _selected_thread_button->set_background_color(t->color());
        _selected_thread_button->set_caption("  ");
    }
};

// Returns true if the mouse coordinates provided intersect with this window
bool ToolWindow::mouse_over(Vector2i position) {
    Vector2i _pos = absolute_position();
    Vector2i _size = size();

    return position[0] >= _pos[0] && position[1] >= _pos[1] && \
           position[0] <= _pos[0] + _size[0] && position[1] <= _pos[1] + _size[1];
};