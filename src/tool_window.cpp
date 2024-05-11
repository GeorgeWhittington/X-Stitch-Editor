#include <nanogui/nanogui.h>
#include <iostream>
#include <fmt/core.h>

#include "tool_window.hpp"
#include "x_stitch_editor.hpp"
#include "project.hpp"
#include "canvas_renderer.hpp"
#include "constants.hpp"
#include "camera2d.hpp"

#define MAKE_TOOLBUTTON_CALLBACK(tool, cursor) [&] {                        \
        _app->_selected_tool = tool;                                        \
        _app->set_cursor(cursor);                                           \
        _app->_previous_backstitch_point = NO_SUBSTITCH_SELECTED;           \
    }                                                                       \

using namespace nanogui;
using Anchor = AdvancedGridLayout::Anchor;

void PaletteButton::palettebutton_callback() {
    _app->_selected_thread = _thread;
    _app->tool_window->update_selected_thread_widget();
    _app->_canvas_renderer->move_ghost_backstitch(_app->_canvas_renderer->_selected_sub_stitch, _thread);
};

// void EditPaletteButton::palettebutton_callback() {
//     for (Thread *t : _app->_project->palette) {
//         if (t == _thread)
//             return;
//     }
//     _app->_project->palette.push_back(_thread);
//     _app->tool_window->set_palette();
//     _app->tool_window->_add_to_palette_button->set_pushed(false);

//     if (_app->_selected_thread == nullptr) {
//         _app->_selected_thread = _thread;
//         _app->tool_window->update_selected_thread_widget();
//     }

//     _app->perform_layout();
// };

void DeletePaletteButton::palettebutton_callback() {
    try {
        _app->_project->remove_from_palette(_thread);
    } catch (std::invalid_argument& err) {
        std::cout << err.what() << std::endl;
    }

    // _app->tool_window->_edit_palette_button->set_pushed(false);
    // _app->tool_window->set_palette();
}

void ToolWindow::initialise() {
    set_position(Vector2i(0, 0));
    set_layout(new GroupLayout(5, 5, 10, 0));

    // Drawing Tools
    Label *label = new Label(this, "Tools", "sans-bold");

    Widget *tool_wrapper = new Widget(this);
    tool_wrapper->set_layout(new BoxLayout(
        Orientation::Horizontal, Alignment::Fill, 0, 0));

    Widget *tools = new Widget(tool_wrapper);
    AdvancedGridLayout *tool_layout = new AdvancedGridLayout(
        std::vector{0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0}, std::vector{0, 5, 0}, 0);
    tools->set_layout(tool_layout);

    auto move_callback = MAKE_TOOLBUTTON_CALLBACK(ToolOptions::MOVE, Cursor::Hand);

    ToolButton *toolbutton = new ToolButton(tools, FA_HAND_PAPER);
    toolbutton->set_tooltip("Move");
    toolbutton->set_callback(move_callback);
    toolbutton->set_pushed(true);
    tool_layout->set_anchor(toolbutton, Anchor(0, 0));
    move_callback();

    toolbutton = new ToolButton(tools, FA_PENCIL_ALT);
    toolbutton->set_tooltip("Single Stitch");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::SINGLE_STITCH, Cursor::Arrow));
    tool_layout->set_anchor(toolbutton, Anchor(2, 0));

    toolbutton = new ToolButton(tools, FA_PEN_NIB);
    toolbutton->set_tooltip("Back Stitch");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::BACK_STITCH, Cursor::Crosshair));
    tool_layout->set_anchor(toolbutton, Anchor(4, 0));

    toolbutton = new ToolButton(tools, FA_ERASER);
    toolbutton->set_tooltip("Erase");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::ERASE, Cursor::Arrow));
    tool_layout->set_anchor(toolbutton, Anchor(6, 0));

    toolbutton = new ToolButton(tools, FA_FILL_DRIP);
    toolbutton->set_tooltip("Fill Area");
    toolbutton->set_callback(MAKE_TOOLBUTTON_CALLBACK(ToolOptions::FILL, Cursor::Arrow));
    tool_layout->set_anchor(toolbutton, Anchor(8, 0));

    toolbutton = new ToolButton(tools, FA_SEARCH_PLUS);
    toolbutton->set_tooltip("Zoom In");
    toolbutton->set_flags(Button::Flags::NormalButton);
    toolbutton->set_callback([this]() { zoom_in(); });
    tool_layout->set_anchor(toolbutton, Anchor(10, 0));

    toolbutton = new ToolButton(tools, FA_SEARCH_MINUS);
    toolbutton->set_tooltip("Zoom Out");
    toolbutton->set_flags(Button::Flags::NormalButton);
    toolbutton->set_callback([this]() { zoom_out(); });
    tool_layout->set_anchor(toolbutton, Anchor(0, 2));

    toolbutton = new ToolButton(tools, FA_UNDO);
    toolbutton->set_tooltip("Undo");
    toolbutton->set_flags(Button::Flags::NormalButton);
    // TODO: callback
    tool_layout->set_anchor(toolbutton, Anchor(2, 2));

    toolbutton = new ToolButton(tools, FA_REDO);
    toolbutton->set_tooltip("Redo");
    toolbutton->set_flags(Button::Flags::NormalButton);
    // TODO: callback
    tool_layout->set_anchor(toolbutton, Anchor(4, 2));

    // Palette
    label = new Label(this, "Selected Thread", "sans-bold");

    _selected_thread_button = new DisabledButton(this, "");
    _selected_thread_theme = new Theme(_app->nvg_context());
    _selected_thread_theme->m_icon_scale = 0.9;
    _selected_thread_button->set_theme(_selected_thread_theme);
    update_selected_thread_widget();

    label = new Label(this, "Palette", "sans-bold");

    // _add_to_palette_button = new PopupButton(this, "Add to Palette");
    // _add_to_palette_button->set_chevron_icon(0);
    // VScrollPanel *popup_scroll = new VScrollPanel(_add_to_palette_button->popup());
    // Widget *popup = new Widget(popup_scroll);
    // popup->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 5, 5));
    // popup->set_fixed_height(500);

    // for (const auto & [manufacturer, man_threads] : _app->_threads) {
    //     Label *popup_label = new Label(popup, manufacturer, "sans-bold");
    //     Widget *man_widget = new Widget(popup);
    //     man_widget->set_layout(new GridLayout(Orientation::Horizontal, 7, Alignment::Maximum, 0, 5));

    //     for (const auto & [key, thread] : *man_threads) {
    //         Widget *thread_widget = new Widget(man_widget);
    //         thread_widget->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 2, 5));
    //         EditPaletteButton *popup_thread = new EditPaletteButton(thread_widget);
    //         popup_thread->set_thread(thread);
    //         popup_thread->set_app(_app);
    //         popup_thread->set_callback();
    //         popup_thread->set_background_color(thread->color());
    //         popup_thread->set_tooltip(thread->description());
    //         popup_label = new Label(thread_widget, thread->number());
    //     }
    // }

    _edit_palette_button = new PopupButton(this, "Edit Palette");
    _edit_palette_button->set_chevron_icon(0);
    VScrollPanel *popup_scroll = new VScrollPanel(_edit_palette_button->popup());
    _edit_palette_widget = new Widget(popup_scroll);
    _edit_palette_widget->set_layout(new GroupLayout(5, 5, 10, 0));

    new Label(_edit_palette_widget, "Add Thread", "sans-bold");
    _add_thread_popup_button = new PopupButton(_edit_palette_widget, "Select Thread");
    _add_thread_popup_button->set_chevron_icon(0);

    _add_blend_thread_popup_button = new PopupButton(_edit_palette_widget, "Select Blend Thread");
    _add_blend_thread_popup_button->set_chevron_icon(0);
    _add_blend_thread_popup_button->set_enabled(false);

    _remove_threads_label = new Label(_edit_palette_widget, "Remove Threads", "sans-bold");
    _remove_threads_label->set_visible(false);
    // TODO: add all current threads inside of remove_threads_widget

    create_themes();
};

void ToolWindow::update_remove_thread_widget() {
    if (_remove_threads_widget != nullptr)
        remove_child(_remove_threads_widget);

    if (_app->_project->palette.size() == 0) {
        _remove_threads_label->set_visible(false);
        _app->perform_layout();
        return;
    }

    _remove_threads_label->set_visible(true);
    _remove_threads_widget = new Widget(_edit_palette_widget);
    GridLayout *layout = new GridLayout(Orientation::Horizontal, 3, Alignment::Fill, 0, 5);
    _remove_threads_widget->set_layout(layout);

    for (Thread *t : _app->_project->palette) {
        new Label(_remove_threads_widget, t->full_name(t->default_position()));
        DisabledButton *colour_button = new DisabledButton(_remove_threads_widget, "  ");
        colour_button->set_background_color(t->color());
        DeletePaletteButton *delete_button = new DeletePaletteButton(_remove_threads_widget, "Delete");
        delete_button->set_thread(t);
        delete_button->set_app(_app);
        delete_button->set_callback();
    }

    _app->perform_layout();
}

void ToolWindow::create_themes() {
    if (_palettebutton_black_text_theme == nullptr || _palettebutton_black_text_theme->m_standard_font_size != 16) {
        _palettebutton_black_text_theme = new Theme(_app->nvg_context());
        _palettebutton_black_text_theme->m_text_color = Color(0, 255);
        _palettebutton_black_text_theme->m_text_color_shadow = Color(255, 255);
    }

    if (_palettebutton_white_text_theme == nullptr || _palettebutton_white_text_theme->m_standard_font_size != 16) {
        _palettebutton_white_text_theme = new Theme(_app->nvg_context());
        _palettebutton_white_text_theme->m_text_color = Color(255, 255);
        _palettebutton_white_text_theme->m_text_color_shadow = Color(0, 255);
    }
}

void ToolWindow::set_palette() {
    if (_palette_container != nullptr) {
        remove_child(_palette_container);
    }

    _palette_container = new VScrollPanel(this);

    std::vector<Thread*> threads = _app->_project->palette;

    if (threads.size() < 1) {
        _app->perform_layout();
        return;
    }

    Widget *palette_widget = new Widget(_palette_container);
    palette_widget->set_layout(new GroupLayout(0, 5, 0, 0));
    for (Thread *t : threads) {
        PaletteButton *button = new PaletteButton(palette_widget);
        button->set_thread(t);
        button->set_app(m_parent);
        button->set_callback();
        button->set_background_color(t->color());
        button->set_tooltip(t->description(t->default_position()));
        button->set_caption(t->full_name(t->default_position()));
        create_themes(); // Check if either theme has been deleted by nanogui reference counting
        if ((t->R * 0.2126f) + (t->G * 0.7152f) + (t->B * 0.0722f) > 179) {
            button->set_theme(_palettebutton_black_text_theme);
        } else {
            button->set_theme(_palettebutton_white_text_theme);
        }
    }

    if (threads.size() > 10)
        _palette_container->set_fixed_height(370);

    _app->perform_layout();

    update_remove_thread_widget();
};

void ToolWindow::update_selected_thread_widget() {
    Thread *t = _app->_selected_thread;
    if (t == nullptr) {
        _selected_thread_button->set_icon(FA_BAN);
        _selected_thread_button->set_background_color(Color(0, 0));
        _selected_thread_button->set_caption("");
        _selected_thread_theme->m_text_color = Color(255, 160);
        _selected_thread_theme->m_text_color_shadow = Color(0, 160);
    } else {
        _selected_thread_button->set_icon(0);
        _selected_thread_button->set_background_color(t->color());

        if ((t->R * 0.2126f) + (t->G * 0.7152f) + (t->B * 0.0722f) > 179) {
            _selected_thread_theme->m_text_color = Color(0, 255);
            _selected_thread_theme->m_text_color_shadow = Color(255, 255);
        } else {
            _selected_thread_theme->m_text_color = Color(255, 255);
            _selected_thread_theme->m_text_color_shadow = Color(0, 255);
        }

        _selected_thread_button->set_tooltip(t->description(t->default_position()));
        _selected_thread_button->set_caption(t->full_name(t->default_position()));
    }
};

// Returns true if the mouse coordinates provided intersect with this window
bool ToolWindow::mouse_over(Vector2i position) {
    Vector2i _pos = absolute_position();
    Vector2i _size = size();

    return position[0] >= _pos[0] && position[1] >= _pos[1] && \
           position[0] <= _pos[0] + _size[0] && position[1] <= _pos[1] + _size[1];
};

void ToolWindow::zoom_in() {
    // TODO: for this and the zoom_out, it would be better to
    // find the location on-screen of the center of the canvas
    // and use zoom_to_point on that position. It's confusing to
    // pan to the side and then be unable to zoom in
    _app->_canvas_renderer->_camera->zoom_camera(
        1.1f, _app->_canvas_renderer->_position);
}

void ToolWindow::zoom_out() {
    _app->_canvas_renderer->_camera->zoom_camera(
        0.9f, _app->_canvas_renderer->_position);
}