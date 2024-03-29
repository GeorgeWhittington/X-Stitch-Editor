#include <nanogui/nanogui.h>

#include "tool_window.hpp"
#include "x_stitch_editor.hpp"

#define MAKE_TOOLBUTTON_CALLBACK(tool, cursor) [&] {                        \
        XStitchEditorApplication* a = (XStitchEditorApplication*) m_parent; \
        a->selected_tool = tool;                                            \
        m_parent->set_cursor(cursor);                                       \
    }                                                                       \

void ToolWindow::initialise() {
    using namespace nanogui;

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
};