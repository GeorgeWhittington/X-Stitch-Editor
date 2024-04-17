#pragma once
#include <nanogui/nanogui.h>
#include <glm/glm.hpp>

class XStitchEditorApplication;

class Camera2D {
public:
    Camera2D(XStitchEditorApplication *app, int width, int height) : _app(app) {
        generate_matrices(true);
    };
    nanogui::Matrix4f generate_matrices(bool first_render = false);
    bool pan_camera(nanogui::Vector2f delta, float position[3*4]);
    bool zoom_camera(float zoom, float position[3*4]);
    void zoom_to_point(nanogui::Vector2f point, float zoom, float position[3*4]);

    float get_scale() { return _scale; };

    nanogui::Matrix4f generate_nanogui_matrix(nanogui::Vector2f translation, float scale, float device_ratio);
    glm::mat4 generate_glm_matrix(nanogui::Vector2f translation, float scale, float device_ratio);
    float calculate_base_scale(int device_height);
    /* Finds the outermost points of the cross stitch canvas in ndc coord space
    using the current translation and scale settings
    */
    nanogui::Vector4f canvas_bounds(float position[3*4]);
    /* Finds the outermost points of the cross stitch canvas in ndc coord space
    using the provided translation and scale settings
    */
    nanogui::Vector4f translated_canvas_bounds(float position[3*4], nanogui::Vector2f translation, float zoom);
    // Determines if the cross stitch canvas bounds provided are adequately visible
    bool valid_bounds(nanogui::Vector4f bounds);
    /* Returns the normalized device coordinates of a specified point on
    the screen. An error is thrown if the point is outside the bounds of the window
    */
    nanogui::Vector2f screen_to_ndc(nanogui::Vector2i screen_coords);
    /* Returns the normalized device coordinates of a specified point on
    the screen, which is then scaled as it would be by the orthographic
    projection. An error is thrown if the point is outside the bounds of the window
    */
    nanogui::Vector2f screen_to_ortho_ndc(nanogui::Vector2i screen_coords);
    // Scales device size by the pixel_ratio of the screen (to handle apple retina screens)
    nanogui::Vector2i get_scaled_device_dimensions();
    bool device_coords_in_bounds(nanogui::Vector2i screen_coords);
    // Determine if an ndc coordinate is within the canvas bounds provided
    bool inside_canvas(nanogui::Vector2f coords, nanogui::Vector4f bounds);
    /* Converts an ndc coordinate to a canvas stitch coordinate
    (Raising an error if out of bounds)
    */
    nanogui::Vector2i ndc_to_stitch(nanogui::Vector2f coords, nanogui::Vector4f bounds);
    /* Converts an ndc coordinate to a canvas substitch coordinate
    (Raising an error if out of bounds)
    */
    nanogui::Vector2f ndc_to_substitch(nanogui::Vector2f coords, nanogui::Vector4f bounds);

private:
    XStitchEditorApplication *_app;
    float _translation_x = 0.f;
    float _translation_y = 0.f;
    float _scale = 1.f;
    float _minimum_scale;
    float _maximum_scale;
    nanogui::Vector2i _device_size;
};