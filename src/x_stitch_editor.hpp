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
class SplashScreenWindow;
class NewProjectWindow;
class CanvasRenderer;
struct Project;

class XStitchEditorApplication : public nanogui::Screen {
private:
// shader
// project
    CanvasRenderer *_canvas_renderer;

    float _last_frame = 0.0f;
    float _time_delta = 0.0f;
public:
    XStitchEditorApplication();
    void load_all_threads();
    void switch_project(Project *project);
    virtual void draw(NVGcontext *ctx);
    virtual void draw_contents();
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers);
    virtual bool scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel);
    virtual bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers);
    virtual bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers);
    virtual bool resize_event(const nanogui::Vector2i &size);

    ToolWindow *tool_window;
    MousePositionWindow *mouse_position_window;
    SplashScreenWindow *splashscreen_window;
    NewProjectWindow *new_project_window;

    Project *_project;
    ToolOptions _selected_tool = ToolOptions::MOVE;
    Thread *_selected_thread;
    std::map<std::string, std::map<std::string, Thread*>*> threads;
};