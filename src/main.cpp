#if defined(NANOGUI_USE_OPENGL)
#  if defined(NANOGUI_GLAD)
#    if defined(NANOGUI_SHARED) && !defined(GLAD_GLAPI_EXPORT)
#      define GLAD_GLAPI_EXPORT
#    endif
#    include <glad/glad.h>
#  else
#    if defined(__APPLE__)
#      define GLFW_INCLUDE_GLCOREARB
#    else
#      define GL_GLEXT_PROTOTYPES
#    endif
#  endif
#elif defined(NANOGUI_USE_GLES)
#  define GLFW_INCLUDE_ES2
#endif

#include <iostream>

#include <GLFW/glfw3.h>
#include <nanogui/nanogui.h>

#include "x_stitch_editor.hpp"
#include "threads.hpp"

nanogui::ref<XStitchEditorApplication> app;

int main(int, char **) {
    glfwInit();
    glfwSetTime(0);

#if defined(NANOGUI_USE_OPENGL)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#elif defined(NANOGUI_USE_GLES)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#elif defined(NANOGUI_USE_METAL)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    nanogui::metal_init();
#endif

    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(1024, 748, "X Stitch Editor", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

#if defined(NANOGUI_GLAD)
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
        throw std::runtime_error("Could not initialize GLAD!");
    glGetError(); // pull and ignore unhandled errors like GL_INVALID_ENUM
#endif

    app = new XStitchEditorApplication();
    app->initialize(window, true);

#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glfwSwapInterval(0);
    glfwSwapBuffers(window);
#endif

    app->create_windows();
    app->set_visible(true);
    app->clear();
    app->draw_all();

    glfwSetCursorPosCallback(window,
        [](GLFWwindow *, double x, double y) {
            app->cursor_pos_callback_event(x, y);
        }
    );

    glfwSetMouseButtonCallback(window,
        [](GLFWwindow *, int button, int action, int modifiers) {
            app->mouse_button_callback_event(button, action, modifiers);
        }
    );

    glfwSetKeyCallback(window,
        [](GLFWwindow *, int key, int scancode, int action, int mods) {
            app->key_callback_event(key, scancode, action, mods);
        }
    );

    glfwSetCharCallback(window,
        [](GLFWwindow *, unsigned int codepoint) {
            app->char_callback_event(codepoint);
        }
    );

    glfwSetDropCallback(window,
        [](GLFWwindow *, int count, const char **filenames) {
            app->drop_callback_event(count, filenames);
        }
    );

    glfwSetScrollCallback(window,
        [](GLFWwindow *, double x, double y) {
            app->scroll_callback_event(x, y);
       }
    );

    glfwSetFramebufferSizeCallback(window,
        [](GLFWwindow *, int width, int height) {
            app->resize_callback_event(width, height);
        }
    );

    nanogui::mainloop(1 / 60.f * 1000);

    // TODO: ensure all windows + data is freed appropriately

    app.reset();

    glfwTerminate();

#if defined(NANOGUI_USE_METAL)
    nanogui::metal_shutdown();
#endif

    return 0;
}