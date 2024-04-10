#include <functional>
#include <iostream>
#include <exception>

#include <nanogui/nanogui.h>
#include <nanogui/opengl.h>

#include "x_stitch_editor.hpp"
#include "tool_window.hpp"
#include "mouse_position_window.hpp"
#include "splashscreen_window.hpp"
#include "new_project_window.hpp"
#include "main_menu_window.hpp"
#include "canvas_renderer.hpp"
#include "camera2d.hpp"
#include "threads.hpp"
#include "project.hpp"

using namespace nanogui;

XStitchEditorApplication::XStitchEditorApplication() : nanogui::Screen(Vector2i(1024, 748), "X Stitch Editor", true) {
    load_all_threads();

    LargeIconTheme *theme = new LargeIconTheme(nvg_context());
    set_theme(theme);

    tool_window = new ToolWindow(this);
    tool_window->initialise();

    mouse_position_window = new MousePositionWindow(this);
    mouse_position_window->initialise();

    splashscreen_window = new SplashScreenWindow(this);
    splashscreen_window->initialise();

    new_project_window = new NewProjectWindow(this);
    new_project_window->initialise();

    main_menu_window = new MainMenuWindow(this);
    main_menu_window->initialise();

    switch_application_state(ApplicationStates::LAUNCH);

    perform_layout();
}

void XStitchEditorApplication::load_all_threads() {
    std::map<std::string, Thread*> *dmc_threads = new std::map<std::string, Thread*>;
    load_manufacturer("/Users/george/Documents/uni_year_three/Digital Systems Project/X-Stitch-Editor/assets/DMC.xml", dmc_threads);
    threads["DMC"] = dmc_threads;
};

void XStitchEditorApplication::switch_project(Project *project) {
    // TODO: consider if this should be achieved with smart pointers?
    // is it bad that canvas_renderer depends on project and these could be
    // changed independently?
    if (_project != nullptr) {
        delete _project;
        _project = nullptr;
    }

    if (_canvas_renderer != nullptr) {
        delete _canvas_renderer;
        _canvas_renderer = nullptr;
    }

    if (project != nullptr) {
        _project = project;
        _canvas_renderer = new CanvasRenderer(this);
    }
}

void XStitchEditorApplication::switch_application_state(ApplicationStates state) {
    switch (state) {
        case ApplicationStates::LAUNCH:
            splashscreen_window->set_visible(true);

            tool_window->set_visible(false);
            mouse_position_window->set_visible(false);
            new_project_window->set_visible(false);
            main_menu_window->set_visible(false);
            break;

        case ApplicationStates::CREATE_PROJECT:
            new_project_window->set_visible(true);

            splashscreen_window->set_visible(false);
            tool_window->set_visible(false);
            mouse_position_window->set_visible(false);
            main_menu_window->set_visible(false);
            break;

        case ApplicationStates::PROJECT_OPEN:
            tool_window->set_visible(true);
            main_menu_window->set_visible(true);

            new_project_window->set_visible(false);
            splashscreen_window->set_visible(false);
            break;

        case ApplicationStates::CREATE_DITHERED_PROJECT:
            // TODO
            break;

        case ApplicationStates::LOAD_PROJECT:
            // TODO
            break;

        default:
            break;
    }
    perform_layout();
}

void XStitchEditorApplication::draw(NVGcontext *ctx) {
    double current_frame = glfwGetTime();
    _time_delta = current_frame - _last_frame;
    _last_frame = current_frame;

    // TODO: iterate visible windows, calculate if they are out of view
    // if they are, put them back to their default position

    /* Draw the user interface */
    Screen::draw(ctx);
};

void XStitchEditorApplication::draw_contents() {
    if (_canvas_renderer == nullptr) {
        Screen::draw_contents();
        return;
    }

    _canvas_renderer->render();

    if (_canvas_renderer->_selected_stitch != NO_STITCH_SELECTED) {
        // TODO: Find thread selected from _canvas_renderer data array instead
        Thread black = {"DMC", "310", "Black", 0, 0, 0};

        mouse_position_window->set_captions(
            _canvas_renderer->_selected_stitch[0] + 1,
            _canvas_renderer->_selected_stitch[1] + 1,
            &black);
    } else {
        mouse_position_window->set_visible(false);
    }
    perform_layout();
};

bool XStitchEditorApplication::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboard_event(key, scancode, action, modifiers))
        return true;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        set_visible(false);
        return true;
    }

    if (_canvas_renderer == nullptr)
        return false;

    Camera2D *camera = _canvas_renderer->_camera;
    float camera_speed = 2 * _time_delta;

    if (key == GLFW_KEY_LEFT)
        camera->pan_camera(Vector2f(camera_speed, 0), _canvas_renderer->_position);
    if (key == GLFW_KEY_RIGHT)
        camera->pan_camera(Vector2f(-camera_speed, 0), _canvas_renderer->_position);
    if (key == GLFW_KEY_UP)
        camera->pan_camera(Vector2f(0, -camera_speed), _canvas_renderer->_position);
    if (key == GLFW_KEY_DOWN)
        camera->pan_camera(Vector2f(0, camera_speed), _canvas_renderer->_position);

#if defined(__APPLE__)
    auto control_command_key = GLFW_MOD_SUPER;
#else
    auto control_command_key = GLFW_MOD_CONTROL;
#endif

    if (key == GLFW_KEY_EQUAL && modifiers == control_command_key)
        camera->zoom_to_point(Vector2f(0, 0), 1.f + camera_speed, _canvas_renderer->_position);
    if (key == GLFW_KEY_MINUS && modifiers == control_command_key)
        camera->zoom_to_point(Vector2f(0, 0), 1.f - camera_speed, _canvas_renderer->_position);

    return false;
}

bool XStitchEditorApplication::scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) {
    if (Screen::scroll_event(p, rel))
        return true;

    if (_canvas_renderer == nullptr)
        return false;

    // We only care about vertical scroll
    if (rel[1] != 0.f) {
        nanogui::Vector2f mouse_ndc = _canvas_renderer->_camera->screen_to_ortho_ndc(p);
        float zoom_factor = 1.f + (rel[1] *_time_delta);
        _canvas_renderer->_camera->zoom_to_point(mouse_ndc, zoom_factor, _canvas_renderer->_position);
    }

    return false;
}

// TODO: start tracking mouse location and only enable the cursor for a specific tool where it
// makes sense. eg:
// single stitch tool: when outside canvas bounds, enable hand cursor
// (ditto for back stitch and fill)

bool XStitchEditorApplication::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) {
    if (Widget::mouse_button_event(p, button, down, modifiers))
        return true;

    if (_canvas_renderer == nullptr)
        return false;

    if (button == GLFW_MOUSE_BUTTON_1 && down) {
        Camera2D *camera = _canvas_renderer->_camera;
        Vector2f mouse_ndc;

        try {
            mouse_ndc = camera->screen_to_ndc(p);
        } catch (std::invalid_argument&) {
            return false;
        }

        if (_selected_tool == ToolOptions::ZOOM_IN) {
            camera->zoom_to_point(mouse_ndc, 1.1f, _canvas_renderer->_position);
            return false;
        }
        if (_selected_tool == ToolOptions::ZOOM_OUT) {
            camera->zoom_to_point(mouse_ndc, 0.9f, _canvas_renderer->_position);
            return false;
        }

        if (tool_window->mouse_over(p))
            return false;

        Vector4f bounds = camera->canvas_bounds(_canvas_renderer->_position);
        Vector2i selected_stitch;

        try {
            selected_stitch = camera->ndc_to_stitch(mouse_ndc, bounds);
        } catch (std::invalid_argument&) {
            return false;
        }

        switch(_selected_tool) {
            case ToolOptions::SINGLE_STITCH:
                if (_selected_thread != nullptr)
                    _canvas_renderer->draw_to_canvas(selected_stitch, _selected_thread);
                break;
            case ToolOptions::BACK_STITCH:
                // TODO
                break;
            case ToolOptions::ERASE:
                _canvas_renderer->erase_from_canvas(selected_stitch);
                break;
            case ToolOptions::FILL:
                // TODO
                break;
            default:
                break;
        }
    }

    return false;
}

bool XStitchEditorApplication::mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) {
    if (Widget::mouse_motion_event(p, rel, button, modifiers))
        return true;

    if (_canvas_renderer == nullptr)
        return false;

    // TODO: check that this gets the right result on windows/linux and
    // that it isn't just a weird glfw quirk
#if defined(__APPLE__)
    int button_1 = GLFW_MOUSE_BUTTON_2;
#else
    int button_1 = GLFW_MOUSE_BUTTON_1;
#endif

    Camera2D *camera = _canvas_renderer->_camera;

    if (button == button_1) {
        Vector2f mouse_ndc;
        try {
            mouse_ndc = camera->screen_to_ndc(p);
        } catch (const std::invalid_argument&) {
            return false;
        }

        if (_selected_tool == ToolOptions::MOVE) {
            Vector2f prev_position = Vector2f(p[0] - rel[0], p[1] - rel[1]);
            Vector2f prev_ndc;
            Vector2f current_ndc;
            try {
                prev_ndc = camera->screen_to_ortho_ndc(prev_position);
                current_ndc = camera->screen_to_ortho_ndc(p);
            } catch (const std::invalid_argument&) {
                return false;
            }

            camera->pan_camera(Vector2f(current_ndc[0] - prev_ndc[0], current_ndc[1] - prev_ndc[1]), _canvas_renderer->_position);
        }

        if (tool_window->mouse_over(p))
            return false;

        Vector4f bounds = camera->canvas_bounds(_canvas_renderer->_position);
        Vector2i selected_stitch;

        try {
            selected_stitch = camera->ndc_to_stitch(mouse_ndc, bounds);
        } catch (std::invalid_argument&) {
            // TODO: if no stitch is selected, and a drawing tool is active, it
            // would make sense to also move the camera here aswell. Should change
            // cursor during duration of the drag to make this obvious.
            return false;
        }

        switch(_selected_tool) {
            case ToolOptions::SINGLE_STITCH:
                if (_selected_thread != nullptr)
                    _canvas_renderer->draw_to_canvas(selected_stitch, _selected_thread);
                break;
            case ToolOptions::ERASE:
                _canvas_renderer->erase_from_canvas(selected_stitch);
            default:
                break;
        }
    }

    return false;
}

bool XStitchEditorApplication::resize_event(const nanogui::Vector2i &size) {
    splashscreen_window->center();
    new_project_window->center();
    main_menu_window->position_top_left();
    return true;
}