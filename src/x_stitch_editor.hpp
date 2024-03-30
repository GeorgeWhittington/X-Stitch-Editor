#pragma once
#include <nanogui/nanogui.h>

#include "constants.hpp"

// state machine with enums for flow through app states (splashscreen, create project, project is open etc)

class LargeIconTheme : public nanogui::Theme {
public:
    LargeIconTheme(NVGcontext *ctx) : nanogui::Theme(ctx) {
        m_icon_scale = 0.9;
    }
};

class ToolWindow;

class XStitchEditorApplication : public nanogui::Screen {
private:
// selected thread

// shader
// project
// canvas renderer

    float _last_frame = 0.0f;
    float _delta_frame = 0.0f;

    ToolWindow *tool_window;
public:
    XStitchEditorApplication();

    ToolOptions selected_tool = ToolOptions::MOVE;
};