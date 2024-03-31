#pragma once
#include <map>
#include <string>
#include <nanogui/nanogui.h>

#include "constants.hpp"
#include "threads.hpp"

// state machine with enums for flow through app states (splashscreen, create project, project is open etc)

class LargeIconTheme : public nanogui::Theme {
public:
    LargeIconTheme(NVGcontext *ctx) : nanogui::Theme(ctx) {
        m_icon_scale = 0.9;
    }
};

class ToolWindow;
class MousePositionWindow;

class XStitchEditorApplication : public nanogui::Screen {
private:
// selected thread

// shader
// project
// canvas renderer

    float _last_frame = 0.0f;
    float _delta_frame = 0.0f;
public:
    XStitchEditorApplication();
    void load_all_threads();

    ToolWindow *tool_window;
    MousePositionWindow *mouse_position_window;

    ToolOptions selected_tool = ToolOptions::MOVE;
    Thread *selected_thread;
    std::map<std::string, std::map<std::string, Thread*>*> threads;
};