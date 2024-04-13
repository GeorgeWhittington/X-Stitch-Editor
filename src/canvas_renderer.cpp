#include "canvas_renderer.hpp"
#include <memory>
#include <algorithm>
#include <iostream>
#include <exception>
#include <queue>
#include <set>
#include <nanogui/nanogui.h>
#include "x_stitch_editor.hpp"
#include "camera2d.hpp"
#include "project.hpp"
#include "threads.hpp"

using namespace nanogui;

struct Vector2iCompare {
    bool operator() (const Vector2i& lhs, const Vector2i& rhs) const {
        return lhs[0] + lhs[1] < rhs[0] + rhs[1];
    }
};

CanvasRenderer::CanvasRenderer(XStitchEditorApplication *app) {
    _app = app;
    int width = app->_project->width;
    int height = app->_project->height;
    _camera = new Camera2D(_app, width, height);

    _texture = new Texture(
        Texture::PixelFormat::RGBA, Texture::ComponentFormat::UInt8,
        Vector2i(width, height),
        Texture::InterpolationMode::Bilinear, Texture::InterpolationMode::Nearest);
    _texture->upload(_app->_project->texture_data_array.get());

    _render_pass = new RenderPass({ app });
    _render_pass->set_clear_color(0, Color(0.3f, 0.3f, 0.32f, 1.f));
    _render_pass->set_cull_mode(RenderPass::CullMode::Disabled);

    _cross_stitch_shader = new Shader(_render_pass, "cross_stitches", CROSS_STITCH_VERT, CROSS_STITCH_FRAG);
    _minor_grid_shader = new Shader(_render_pass, "minor_grid", GRID_VERT, GRID_FRAG);
    _major_grid_shader = new Shader(_render_pass, "major_grid", GRID_VERT, GRID_FRAG);

    float h = 1.f;
    float v = 1.f;

    if (width > height) {
        v = (float)height / (float)width;
    } else {
        h = (float)width / (float)height;
    }

    _canvas_height_ndc = v * 2.f;

    float position[3*4] = {
        -h, -v, 0,
         h, -v, 0,
         h,  v, 0,
        -h,  v, 0
    };
    std::copy(position, position + (3 * 4), _position);
    float tex[2*4] = {
        0, 0,  // bottom left
        1, 0,  // bottom right
        1, 1,  // top right
        0, 1   // top left
    };
    uint32_t indices[3*2] = {
        0, 1, 2,
        2, 3, 0
    };

    _cross_stitch_shader->set_buffer("position", VariableType::Float32, {4, 3}, _position);
    _cross_stitch_shader->set_buffer("tex", VariableType::Float32, {4, 2}, tex);
    _cross_stitch_shader->set_buffer("indices", VariableType::UInt32, {3*2}, indices);
    _cross_stitch_shader->set_texture("cross_stitch_texture", _texture);

    _minor_grid_mark_distance = 2.f / (float)std::max(width, height);
    float major_grid_mark_distance = _minor_grid_mark_distance * 10.f;
    float major_grid_mark_normal = _minor_grid_mark_distance * 0.125f;

    int width_minus_1 = width - 1;
    int height_minus_1 = height - 1;

    _minor_grid_total_verts = (height_minus_1 * 4) + (width_minus_1 * 4);
    int major_grid_total_verts = ((height_minus_1 / 10) * 8) + ((width_minus_1 / 10) * 8);
    float minor_gridmarks[_minor_grid_total_verts];
    float major_gridmarks[major_grid_total_verts];

    float x = -h;
    for (int i = 0; i < width_minus_1; i++) {
        x += _minor_grid_mark_distance;
        // two points, forming a line
        minor_gridmarks[4*i] =        x;
        minor_gridmarks[(4*i) + 1] =  v;
        minor_gridmarks[(4*i) + 2] =  x;
        minor_gridmarks[(4*i) + 3] = -v;
    }

    float y = -v;
    for (int i = width_minus_1; i < height_minus_1 + width_minus_1; i++) {
        y += _minor_grid_mark_distance;
        minor_gridmarks[4*i] =        h;
        minor_gridmarks[(4*i) + 1] =  y;
        minor_gridmarks[(4*i) + 2] = -h;
        minor_gridmarks[(4*i) + 3] =  y;
    }

    x = -h;
    for (int i = 0; i < (width_minus_1 / 10); i++) {
        x += major_grid_mark_distance;
        // four points, forming a line when drawn as a long rectangle
        major_gridmarks[8*i]       = x - major_grid_mark_normal;
        major_gridmarks[(8*i) + 1] = v;
        major_gridmarks[(8*i) + 2] = x - major_grid_mark_normal;
        major_gridmarks[(8*i) + 3] = -v;
        major_gridmarks[(8*i) + 4] = x + major_grid_mark_normal;
        major_gridmarks[(8*i) + 5] = v;
        major_gridmarks[(8*i) + 6] = x + major_grid_mark_normal;
        major_gridmarks[(8*i) + 7] = -v;
    }

    y = -v;
    for (int i = (width_minus_1 / 10); i < (height_minus_1 / 10) + (width_minus_1 / 10); i++) {
        y += major_grid_mark_distance;
        major_gridmarks[8*i]       = h;
        major_gridmarks[(8*i) + 1] = y - major_grid_mark_normal;
        major_gridmarks[(8*i) + 2] = -h;
        major_gridmarks[(8*i) + 3] = y - major_grid_mark_normal;
        major_gridmarks[(8*i) + 4] = h;
        major_gridmarks[(8*i) + 5] = y + major_grid_mark_normal;
        major_gridmarks[(8*i) + 6] = -h;
        major_gridmarks[(8*i) + 7] = y + major_grid_mark_normal;
    }

    _major_grid_indices_size = (major_grid_total_verts / 4) * 6;
    uint8_t major_grid_indices[_major_grid_indices_size];
    int i = 0;
    for (int j = 0; j < major_grid_total_verts; j+=4) {
        major_grid_indices[6*i]       = j + 0;
        major_grid_indices[(6*i) + 1] = j + 1;
        major_grid_indices[(6*i) + 2] = j + 2;
        major_grid_indices[(6*i) + 3] = j + 2;
        major_grid_indices[(6*i) + 4] = j + 1;
        major_grid_indices[(6*i) + 5] = j + 3;
        i++;
    };

    float minor_color[4] = {0.5f, 0.5f, 0.5f, 1.f};
    float major_color[4] = {0.f, 0.f, 0.f, 1.f};

    _minor_grid_shader->set_buffer("position", VariableType::Float32, {(size_t)(_minor_grid_total_verts) / 2, 2}, minor_gridmarks);
    _minor_grid_shader->set_buffer("colour", VariableType::Float32, {4}, minor_color);

    if (major_grid_total_verts > 0) {
        _major_grid_shader->set_buffer("position", VariableType::Float32, {(size_t)(major_grid_total_verts) / 2, 2}, major_gridmarks);
        _major_grid_shader->set_buffer("indices", VariableType::UInt8, {(size_t)_major_grid_indices_size}, major_grid_indices);
        _major_grid_shader->set_buffer("colour", VariableType::Float32, {4}, major_color);
    } else {
        delete _major_grid_shader;
    }
};

Vector2i CanvasRenderer::get_mouse_position() {
    Vector4f bounds = _camera->canvas_bounds(_position);
    try {
        Vector2f mouse_ndc = _camera->screen_to_ndc(_app->mouse_pos());
        return _camera->ndc_to_stitch(mouse_ndc, bounds);
    } catch (std::invalid_argument&) {
        return NO_STITCH_SELECTED;
    }
}

Thread* CanvasRenderer::find_thread_at_position(Vector2i stitch) {
    int palette_id = _app->_project->thread_data[stitch[0]][stitch[1]];
    try {
        Thread *t = _app->_project->palette.at(palette_id);
        return t;
    } catch (std::out_of_range&) {
        // this shouldn't happen, but to be safe clear the stitch
        // that contains a thread we don't know
        erase_from_canvas(stitch);
        return nullptr;
    }
}

// TODO: Should app just call project.draw_stitch() directly and then
// just call a method that uploads the texture?
void CanvasRenderer::draw_to_canvas(Vector2i stitch, Thread *thread) {
    _app->_project->draw_stitch(stitch, thread);
    _texture->upload(_app->_project->texture_data_array.get());
}

// TODO: ditto above
void CanvasRenderer::erase_from_canvas(Vector2i stitch) {
    _app->_project->erase_stitch(stitch);
    _texture->upload(_app->_project->texture_data_array.get());
}

void CanvasRenderer::fill_from_point(Vector2i stitch, Thread *thread) {
    Thread *target_thread = find_thread_at_position(stitch);

    std::queue<Vector2i> unvisited;
    std::set<Vector2i, Vector2iCompare> visited;

    unvisited.push(stitch);

    while(unvisited.size() > 0) {
        Vector2i current = unvisited.front();
        Thread *current_thread = find_thread_at_position(current);

        if (current_thread != target_thread) {
            // not part of fill area, don't change this or look at its surrounding pixels
            unvisited.pop();
            continue;
        }

        Vector2i touching[4] = {
            {current[0] - 1, current[1]},
            {current[0] + 1, current[1]},
            {current[0], current[1] + 1},
            {current[0], current[1] - 1}
        };

        for (const Vector2i& s : touching) {
            if (_app->_project->is_stitch_valid(s) && !visited.contains(s))
                unvisited.push(s);
        }

        _app->_project->draw_stitch(current, thread);

        unvisited.pop();
    }
    _texture->upload(_app->_project->texture_data_array.get());
}

void CanvasRenderer::render() {
    Vector2i device_size = _app->framebuffer_size();
    _render_pass->resize(device_size);

    _selected_stitch = get_mouse_position();

    float pixel_canvas_height = _canvas_height_ndc * _camera->get_scale() * (float)(device_size[1]) / _app->pixel_ratio();
    float minor_grid_mark_pixel_distance = pixel_canvas_height * _minor_grid_mark_distance;

    Matrix4f mvp = _camera->generate_matrices();

    _render_pass->begin();

    render_cs_shader(mvp);
    if (minor_grid_mark_pixel_distance >= 15.f) {
        render_minor_grid_shader(mvp);
        render_major_grid_shader(mvp);
    }

    _render_pass->end();
};

void CanvasRenderer::render_cs_shader(Matrix4f mvp) {
    if (_cross_stitch_shader == nullptr)
        return;

    _cross_stitch_shader->set_uniform("mvp", mvp);

    _cross_stitch_shader->begin();
    _cross_stitch_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 6, true);
    _cross_stitch_shader->end();
};

void CanvasRenderer::render_minor_grid_shader(Matrix4f mvp) {
    if (_minor_grid_shader == nullptr)
        return;

    _minor_grid_shader->set_uniform("mvp", mvp);

    _minor_grid_shader->begin();
    _minor_grid_shader->draw_array(Shader::PrimitiveType::Line, 0,
                                   _minor_grid_total_verts, false);
    _minor_grid_shader->end();
}

void CanvasRenderer::render_major_grid_shader(Matrix4f mvp) {
    if (_major_grid_shader == nullptr)
        return;

    _major_grid_shader->set_uniform("mvp", mvp);

    _major_grid_shader->begin();
    _major_grid_shader->draw_array(Shader::PrimitiveType::Triangle, 0,
                                   _major_grid_indices_size, true);
    _major_grid_shader->end();
}