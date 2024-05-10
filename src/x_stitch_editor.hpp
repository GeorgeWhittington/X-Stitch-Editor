#pragma once
#include <map>
#include <string>

#include <nanogui/nanogui.h>

#include "constants.hpp"
#include "threads.hpp"

class ToolWindow;
class MousePositionWindow;
class SplashScreenWindow;
class NewProjectWindow;
class MainMenuWindow;
class PDFWindow;
class DitheringWindow;
class CanvasRenderer;
struct Project;
class XStitchEditorApplication;

class ExitToMainMenuWindow : public nanogui::Window {
public:
    ExitToMainMenuWindow(nanogui::Widget *parent, const std::string title = "") : nanogui::Window(parent, title) {};
    void initialise();
};

class XStitchEditorApplication : public nanogui::Screen {
private:
    void set_all_windows_invisible();

    ApplicationStates _previous_state = ApplicationStates::LAUNCH;

    float _last_frame = 0.0f;
    float _time_delta = 0.0f;
public:
    XStitchEditorApplication();
    void load_all_threads();
    void switch_project(Project *project);
    void switch_application_state(ApplicationStates state);
    void open_project();
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
    MainMenuWindow *main_menu_window;
    ExitToMainMenuWindow *exit_to_main_menu_window;
    PDFWindow *pdf_window;
    DitheringWindow *dithering_window;

    CanvasRenderer *_canvas_renderer = nullptr;
    Project *_project = nullptr;
    ToolOptions _selected_tool = ToolOptions::MOVE;
    Thread *_selected_thread = nullptr;
    std::map<std::string, std::map<std::string, Thread*>*> _threads;
    nanogui::Vector2f _previous_backstitch_point = NO_SUBSTITCH_SELECTED;
};