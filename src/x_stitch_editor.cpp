#include <functional>
#include <iostream>
#include <exception>

#include <nanogui/nanogui.h>
#include <nanogui/opengl.h>
#include <fmt/core.h>

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
#include "constants.hpp"

using namespace nanogui;

std::vector<std::pair<std::string, std::string>> permitted_files = {{"oxs", "Open Cross Stitch"}, {"OXS", "Open Cross Stitch"}};

void ExitToMainMenuWindow::initialise() {
    set_position(Vector2i(10, 10));
    set_layout(new BoxLayout(Orientation::Horizontal));
    Button *b = new Button(this, "", FA_CHEVRON_LEFT);
    b->set_callback([this](){
        auto app = (XStitchEditorApplication*)m_parent;
        app->switch_application_state(ApplicationStates::LAUNCH);
    });
};

XStitchEditorApplication::XStitchEditorApplication() : Screen(Vector2i(1024, 748), "X Stitch Editor", true) {
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

    exit_to_main_menu_window = new ExitToMainMenuWindow(this);
    exit_to_main_menu_window->initialise();

    switch_application_state(ApplicationStates::LAUNCH);

    perform_layout();
}

void XStitchEditorApplication::load_all_threads() {
    // TODO: once installed designate a folder to contain manufacturer.xml files
    // and attempt to load all of those here. If a user adds a new manufacturer file
    // and it is valid then it should be copied to this folder

    std::map<std::string, Thread*> *dmc_threads = new std::map<std::string, Thread*>;
    try {
        std::string manufacturer_name = load_manufacturer("/Users/george/Documents/uni_year_three/Digital Systems Project/X-Stitch-Editor/assets/DMC.xml", dmc_threads);
        _threads[manufacturer_name] = dmc_threads;
    } catch (std::runtime_error& err) {
        delete dmc_threads;
        std::cerr << fmt::format("Could not load thread manufacturer DMC: {}", err.what()) << std::endl;
    }
};

void XStitchEditorApplication::switch_project(Project *project) {
    _selected_thread = nullptr;
    tool_window->update_selected_thread_widget();

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
        _canvas_renderer->update_backstitch_buffers();
        tool_window->set_palette();
    }
}

void XStitchEditorApplication::switch_application_state(ApplicationStates state) {
    if (_previous_state == ApplicationStates::PROJECT_OPEN) {
        main_menu_window->close_all_submenus();
        main_menu_window->_menu_button->set_pushed(false);
    }

    if (_previous_state == ApplicationStates::CREATE_PROJECT)
        new_project_window->reset_form();

    // Reposition all windows to default
    splashscreen_window->center();
    new_project_window->center();
    main_menu_window->position_top_left();

    switch (state) {
        case ApplicationStates::LAUNCH:
            splashscreen_window->set_visible(true);

            tool_window->set_visible(false);
            mouse_position_window->set_visible(false);
            new_project_window->set_visible(false);
            main_menu_window->set_visible(false);
            exit_to_main_menu_window->set_visible(false);
            break;

        case ApplicationStates::CREATE_PROJECT:
            new_project_window->set_visible(true);
            exit_to_main_menu_window->set_visible(true);

            splashscreen_window->set_visible(false);
            tool_window->set_visible(false);
            mouse_position_window->set_visible(false);
            main_menu_window->set_visible(false);
            break;

        case ApplicationStates::PROJECT_OPEN:
            main_menu_window->set_visible(true);
            tool_window->set_visible(true);

            main_menu_window->update_tool_toggle_icon();
            main_menu_window->update_minimap_toggle_icon();

            new_project_window->set_visible(false);
            splashscreen_window->set_visible(false);
            exit_to_main_menu_window->set_visible(false);
            break;

        case ApplicationStates::CREATE_DITHERED_PROJECT:
            // TODO
            break;

        default:
            break;
    }

    _previous_state = state;
    perform_layout();
}

void XStitchEditorApplication::open_project() {
    std::string path = nanogui::file_dialog(permitted_files, false);

    if (path == "")
        return;

    Project *project;

    try {
        project = new Project(path.c_str(), &_threads);
    } catch (const std::runtime_error& err) {
        // TODO: plus all other errors the constructor raises
        // And put the error msg into a popup
        std::cerr << err.what() << std::endl;
        return;
    }

    switch_project(project);
    switch_application_state(ApplicationStates::PROJECT_OPEN);
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
        mouse_position_window->set_captions(
            _canvas_renderer->_selected_stitch[0] + 1,
            _canvas_renderer->_selected_stitch[1] + 1,
            _project->find_thread_at_stitch(_canvas_renderer->_selected_stitch));
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

#if defined(__APPLE__)
    auto control_command_key = GLFW_MOD_SUPER;
#else
    auto control_command_key = GLFW_MOD_CONTROL;
#endif

    // save on ctrl+S or cmd+S
    if (key == GLFW_KEY_S && action == GLFW_PRESS && modifiers & control_command_key) {
        if (_project->file_path == "") {
            std::string path = nanogui::file_dialog(permitted_files, true);

            if (path != "")
                _project->save(path.c_str(), this);
        } else {
            _project->save(_project->file_path.c_str(), this);
        }
    }

    std::shared_ptr<Camera2D> camera = _canvas_renderer->_camera;
    float camera_speed = 2 * _time_delta;

    if (key == GLFW_KEY_LEFT)
        camera->pan_camera(Vector2f(camera_speed, 0), _canvas_renderer->_position);
    if (key == GLFW_KEY_RIGHT)
        camera->pan_camera(Vector2f(-camera_speed, 0), _canvas_renderer->_position);
    if (key == GLFW_KEY_UP)
        camera->pan_camera(Vector2f(0, -camera_speed), _canvas_renderer->_position);
    if (key == GLFW_KEY_DOWN)
        camera->pan_camera(Vector2f(0, camera_speed), _canvas_renderer->_position);

    if (key == GLFW_KEY_EQUAL && modifiers & control_command_key)
        camera->zoom_to_point(Vector2f(0, 0), 1.f + camera_speed, _canvas_renderer->_position);
    if (key == GLFW_KEY_MINUS && modifiers & control_command_key)
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

    if (button != GLFW_MOUSE_BUTTON_1 && button != GLFW_MOUSE_BUTTON_2)
        return false;

    std::shared_ptr<Camera2D> camera = _canvas_renderer->_camera;
    Vector2f mouse_ndc;

    try {
        mouse_ndc = camera->screen_to_ndc(p);
    } catch (std::invalid_argument&) {
        return false;
    }

    if (tool_window->mouse_over(p))
            return false;

    if (button == GLFW_MOUSE_BUTTON_1 && down) {
        if (_selected_tool == ToolOptions::ZOOM_IN) {
            camera->zoom_to_point(mouse_ndc, 1.1f, _canvas_renderer->_position);
            return false;
        }
        if (_selected_tool == ToolOptions::ZOOM_OUT) {
            camera->zoom_to_point(mouse_ndc, 0.9f, _canvas_renderer->_position);
            return false;
        }

        Vector2i selected_stitch = _canvas_renderer->_selected_stitch;
        Vector2f selected_substitch = _canvas_renderer->_selected_sub_stitch;

        if (selected_stitch == NO_STITCH_SELECTED)
            return false;

        switch(_selected_tool) {
            case ToolOptions::SINGLE_STITCH:
                if (_selected_thread != nullptr) {
                    _project->draw_stitch(selected_stitch, _selected_thread);
                    _canvas_renderer->upload_texture();
                }
                break;
            case ToolOptions::BACK_STITCH:
                // ignore input before thread is selected
                if (_selected_thread == nullptr)
                    break;

                // first click, store start point
                if (_previous_backstitch_point == NO_SUBSTITCH_SELECTED) {
                    _previous_backstitch_point = selected_substitch;
                    _canvas_renderer->move_ghost_backstitch(selected_substitch, _selected_thread);
                    break;
                }

                // two clicks to the same location, discard
                if (_previous_backstitch_point == selected_substitch) {
                    _previous_backstitch_point = NO_SUBSTITCH_SELECTED;
                    _canvas_renderer->clear_ghost_backstitch();
                    break;
                }

                _project->draw_backstitch(_previous_backstitch_point, selected_substitch, _selected_thread);
                _previous_backstitch_point = NO_SUBSTITCH_SELECTED;
                _canvas_renderer->clear_ghost_backstitch();
                _canvas_renderer->update_backstitch_buffers();
                break;
            case ToolOptions::ERASE:
                _project->erase_stitch(selected_stitch);
                _canvas_renderer->upload_texture();

                _project->erase_backstitches_intersecting(selected_stitch);
                _canvas_renderer->update_backstitch_buffers();
                break;
            case ToolOptions::FILL:
                _project->fill_from_stitch(selected_stitch, _selected_thread);
                _canvas_renderer->upload_texture();
                break;
            default:
                break;
        }
    } else if (button == GLFW_MOUSE_BUTTON_2 && down) {
        if (_selected_tool == ToolOptions::BACK_STITCH && _previous_backstitch_point != NO_SUBSTITCH_SELECTED) {
            // Clear selection
            _previous_backstitch_point = NO_SUBSTITCH_SELECTED;
            _canvas_renderer->clear_ghost_backstitch();
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

    std::shared_ptr<Camera2D> camera = _canvas_renderer->_camera;

    // Halfway through drawing a backstitch
    if (_selected_tool == ToolOptions::BACK_STITCH && _previous_backstitch_point != NO_SUBSTITCH_SELECTED) {
        Vector2f selected_sub_stitch = _canvas_renderer->_selected_sub_stitch;
        if (selected_sub_stitch == NO_SUBSTITCH_SELECTED) {
            _canvas_renderer->clear_ghost_backstitch();
        } else {
            _canvas_renderer->move_ghost_backstitch(selected_sub_stitch, _selected_thread);
        }
    }

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

        Vector2i selected_stitch = _canvas_renderer->_selected_stitch;

        // TODO: if no stitch is selected, and a drawing tool is active, it
        // would make sense to also move the camera here aswell. Should change
        // cursor during duration of the drag to make this obvious.
        if (selected_stitch == NO_STITCH_SELECTED)
            return false;

        switch(_selected_tool) {
            case ToolOptions::SINGLE_STITCH:
                if (_selected_thread != nullptr) {
                    _project->draw_stitch(selected_stitch, _selected_thread);
                    _canvas_renderer->upload_texture();
                }
                break;
            case ToolOptions::ERASE:
                // TODO: erasing while moving isn't working as I'd expect, look into it
                _project->erase_stitch(selected_stitch);
                _canvas_renderer->upload_texture();

                _project->erase_backstitches_intersecting(selected_stitch);
                _canvas_renderer->update_backstitch_buffers();
                break;
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