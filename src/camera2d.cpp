#include "camera2d.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <nanogui/nanogui.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include "x_stitch_editor.hpp"
#include "project.hpp"

using nanogui::Matrix4f;
using nanogui::Vector2i;
using nanogui::Vector3f;
using nanogui::Vector4f;
using nanogui::Vector2f;

Matrix4f Camera2D::generate_matrices(bool first_render) {
    Vector2i current_device_size = _app->framebuffer_size();
    float device_ratio = (float)(current_device_size[0]) / (float)(current_device_size[1]);

    if (first_render) {
        _device_size = current_device_size;
        _scale = calculate_base_scale(current_device_size[1]);
        _minimum_scale = _scale / 2.f;
        _maximum_scale = _scale * 200;
        _scale *= 2;
    } else if (_device_size != current_device_size) {
        // clamp zoom on window re-size
        _device_size = current_device_size;
        float base_scale = calculate_base_scale(current_device_size[1]);
        _minimum_scale = base_scale / 2;
        _maximum_scale = base_scale * 200;
        _scale = std::max(_minimum_scale, std::min(_maximum_scale, _scale));
    }

    return generate_nanogui_matrix(Vector2f(_translation_x, _translation_y), _scale, device_ratio);
}

bool Camera2D::pan_camera(Vector2f delta, float position[3*4]) {
    float new_x = _translation_x + (delta[0] / _scale);
    float new_y = _translation_y + (delta[1] / _scale);

    Vector4f bounds = translated_canvas_bounds(position, Vector2f(new_x, new_y), _scale);
    if (!valid_bounds(bounds))
        return false;

    _translation_x = new_x;
    _translation_y = new_y;
    return true;
}

bool Camera2D::zoom_camera(float zoom, float position[3*4]) {
    float new_scale = _scale * zoom;

    Vector4f bounds = translated_canvas_bounds(position, Vector2f(_translation_x, _translation_y), zoom);
    if (!valid_bounds(bounds))
        return false;

    _scale = std::max(_minimum_scale, std::min(_maximum_scale, new_scale));
    return true;
}

void Camera2D::zoom_to_point(Vector2f point, float zoom, float position[3*4]) {
    bool success = pan_camera(Vector2f(-point[0], -point[1]), position);
    if (!success)
        return;

    float prev_scale = _scale;
    success = zoom_camera(zoom, position);
    if (!success) {
        // undo all previous steps
        pan_camera(point, position);
        return;
    }

    success = pan_camera(point, position);
    if (!success) {
        // undo all previous steps
        zoom_camera(prev_scale / _scale, position);
        pan_camera(point, position);
    }
}

Matrix4f Camera2D::generate_nanogui_matrix(Vector2f translation, float scale_, float device_ratio) {
    Matrix4f trans = Matrix4f::translate(Vector3f(translation[0], translation[1], 0.f));
    Matrix4f scale = Matrix4f::scale(Vector3f(scale_, scale_, 1.f));
    Matrix4f ortho = Matrix4f::ortho(
        -1 * device_ratio, 1 * device_ratio, // left, right
        -1, 1, -1.f, 1.f                     // bottom, top, near, far
    );

    return ortho * scale * trans;
}

glm::mat4 Camera2D::generate_glm_matrix(Vector2f translation, float scale_, float device_ratio) {
    glm::mat4 trans = glm::translate(glm::mat4(1.f), glm::vec3(translation[0], translation[1], 0.f));
    glm::mat4 scale = glm::scale(glm::mat4(1.f), glm::vec3(scale_, scale_, 1.f));
    glm::mat4 ortho = glm::ortho(
        -1 * device_ratio, 1 * device_ratio, // left, right
        -1.f, 1.f, -1.f, 1.f                 // bottom, top, near, far
    );

    return ortho * scale * trans;
}

float Camera2D::calculate_base_scale(int device_height) {
    return (float)_app->_project->height / (float)device_height * _app->pixel_ratio();
}

Vector4f Camera2D::canvas_bounds(float position[3*4]) {
    return translated_canvas_bounds(position, Vector2f(_translation_x, _translation_y), _scale);
}

Vector4f Camera2D::translated_canvas_bounds(float position[3*4], Vector2f translation, float zoom) {
    Vector2i current_device_size = _app->framebuffer_size();
    float device_ratio = (float)(current_device_size[0]) / (float)(current_device_size[1]);

    glm::mat4 mvp = generate_glm_matrix(translation, zoom, device_ratio);
    // 2nd and 4th vertices from position
    glm::vec4 translated_1 = mvp * glm::vec4(position[3], position[4], position[5], 1.f);
    glm::vec4 translated_2 = mvp * glm::vec4(position[9], position[10], position[11], 1.f);

    // left, right, top, bottom
    return Vector4f(translated_2[0], translated_1[0], translated_2[1], translated_1[1]);
}

bool Camera2D::valid_bounds(Vector4f bounds) {
    // left, right, top, bottom
    return !(bounds[0] > 0.8f || bounds[1] < -0.8f || bounds[2] < -0.8f || bounds[3] > 0.8f);
}

Vector2f Camera2D::screen_to_ndc(Vector2i screen_coords) {
    if (!device_coords_in_bounds(screen_coords))
        throw std::invalid_argument("Coordinates provided are out of bounds");

    Vector2f coords = screen_coords;
    Vector2f device_size = get_scaled_device_dimensions();

    // normalise to -1, 1 https://stats.stackexchange.com/questions/178626/how-to-normalize-data-between-1-and-1
    float normalised_x = (2.f * (coords[0] / device_size[0])) -1.f;
    float normalised_y = (2.f * ((device_size[1] - coords[1]) / device_size[1])) -1.f; // y needs to be inverted

    return Vector2f(normalised_x, normalised_y);
}

Vector2f Camera2D::screen_to_ortho_ndc(Vector2i screen_coords) {
    if (!device_coords_in_bounds(screen_coords))
        throw std::invalid_argument("Coordinates provided are out of bounds");

    Vector2f coords = screen_coords;
    Vector2f device_size = get_scaled_device_dimensions();
    float device_ratio = device_size[0] / device_size[1];

    // normalise x to -device_ratio, device_ratio and y to -1, 1
    float normalised_x = (2.f * device_ratio * ( coords[0] / device_size[0] )) - device_ratio;
    float normalised_y = (2.f * ((device_size[1] - coords[1]) / device_size[1])) - 1.f; // y needs to be inverted

    return Vector2f(normalised_x, normalised_y);
}

Vector2i Camera2D::get_scaled_device_dimensions() {
    Vector2i device_size = _app->framebuffer_size();
    float pixel_ratio = _app->pixel_ratio();
    float device_width = device_size[0] / pixel_ratio;
    float device_height = device_size[1] / pixel_ratio;
    return Vector2i(device_width, device_height);
}

bool Camera2D::device_coords_in_bounds(Vector2i screen_coords) {
    Vector2i device_size = _app->framebuffer_size();
    return screen_coords[0] >= 0 && screen_coords[1] >= 0 && \
           screen_coords[0] <= device_size[0] && screen_coords[1] <= device_size[1];
}

bool Camera2D::inside_canvas(Vector2f coords, Vector4f bounds) {
    // x >= left && x <= right && y >= bottom && y <= top
    return coords[0] >= bounds[0] && coords[0] <= bounds[1] && coords[1] >= bounds[3] && coords[1] <= bounds[2];
}

Vector2i Camera2D::ndc_to_stitch(Vector2f coords, Vector4f bounds) {
    if (!inside_canvas(coords, bounds))
        throw std::invalid_argument("Coordinate outside bounds of canvas");

    int stitch_x = (float)_app->_project->width * ((coords[0] - bounds[0]) / (bounds[1] - bounds[0]));
    int stitch_y = (float)_app->_project->height * ((coords[1] - bounds[3]) / (bounds[2] - bounds[3]));

    stitch_x = std::max(0, std::min(_app->_project->width - 1, stitch_x));
    stitch_y = std::max(0, std::min(_app->_project->height - 1, stitch_y));
    return Vector2i(stitch_x, stitch_y);
}

Vector2f Camera2D::ndc_to_substitch(Vector2f coords, Vector4f bounds) {
    if (!inside_canvas(coords, bounds))
        throw std::invalid_argument("Coordinate outside bounds of canvas");

    float stitch_x = (float)_app->_project->width * ((coords[0] - bounds[0]) / (bounds[1] - bounds[0]));
    float stitch_y = (float)_app->_project->height * ((coords[1] - bounds[3]) / (bounds[2] - bounds[3]));

    stitch_x = std::max(0.f, std::min((float)(_app->_project->width - 1), stitch_x));
    stitch_y = std::max(0.f, std::min((float)(_app->_project->height - 1), stitch_y));

    stitch_x = std::round(stitch_x * 2.f) / 2.f;
    stitch_y = std::round(stitch_y * 2.f) / 2.f;
    return Vector2f(stitch_x, stitch_y);
}