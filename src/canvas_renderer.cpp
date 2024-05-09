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

CanvasRenderer::CanvasRenderer(XStitchEditorApplication *app) :
    _app(app),
    _render_pass(new RenderPass({app})),
    _cross_stitch_shader(new Shader(_render_pass.get(), "cross_stitches", CROSS_STITCH_VERT, CROSS_STITCH_FRAG)),
    _minor_grid_shader(new Shader(_render_pass.get(), "minor_grid", MINOR_GRID_VERT, MINOR_GRID_FRAG)),
    _major_grid_shader(new Shader(_render_pass.get(), "major_grid", MAJOR_GRID_VERT, MAJOR_GRID_FRAG)),
    _back_stitch_shader(new Shader(_render_pass.get(), "back_stitch", BACK_STITCH_VERT, BACK_STITCH_FRAG)),
    _back_stitch_ghost_shader(new Shader(_render_pass.get(), "back_stitch_ghost", BACK_STITCH_VERT, BACK_STITCH_FRAG))
{
    _render_pass->set_clear_color(0, Color(0.3f, 0.3f, 0.32f, 1.f));
    _render_pass->set_cull_mode(RenderPass::CullMode::Disabled);
};

void CanvasRenderer::deactivate() {
    _major_grid_indices_size = 0;
    _minor_grid_indices_size = 0;
    _backstitch_indices_size = 0;
    _backstitch_ghost_indices_size = 0;
    _drawing = false;
}

void CanvasRenderer::update_all_buffers() {
    int width = _app->_project->width;
    int height = _app->_project->height;

    if (_camera != nullptr)
        _camera.release();

    if (_texture != nullptr)
        _texture.release();

    _camera = std::make_unique<Camera2D>(_app, width, height);
    _texture = std::make_unique<Texture>(
        Texture::PixelFormat::RGBA, Texture::ComponentFormat::UInt8,
        Vector2i(width, height),
        Texture::InterpolationMode::Bilinear, Texture::InterpolationMode::Nearest
    );

    upload_texture();

    _v = 1.f;
    _h = 1.f;

    if (width > height) {
        _v = (float)height / (float)width;
    } else {
        _h = (float)width / (float)height;
    }

    _canvas_height_ndc = _v * 2.f;

    float position[3*4] = {
        -_h, -_v, 0,
         _h, -_v, 0,
         _h,  _v, 0,
        -_h,  _v, 0
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
    _cross_stitch_shader->set_texture("cross_stitch_texture", _texture.get());

    _minor_grid_mark_distance = 2.f / (float)std::max(width, height); // ndc space is 2.f long in the axis that is largest
    float major_grid_mark_distance = _minor_grid_mark_distance * 10.f;

    int width_minus_1 = width - 1;
    int height_minus_1 = height - 1;

    int minor_grid_total_verts = (height_minus_1 * 4) + (width_minus_1 * 4);
    int major_grid_total_horizontal_verts = (height_minus_1 / 10) * 8;
    int major_grid_total_vertical_verts = (width_minus_1 / 10) * 8;
    int major_grid_total_verts = major_grid_total_horizontal_verts + major_grid_total_vertical_verts;
    float minor_gridmarks[minor_grid_total_verts];
    float major_gridmarks[major_grid_total_verts];

    float x = -_h;
    for (int i = 0; i < width_minus_1; i++) {
        x += _minor_grid_mark_distance;
        // two points, forming a line
        minor_gridmarks[4*i] =        x;
        minor_gridmarks[(4*i) + 1] =  _v;
        minor_gridmarks[(4*i) + 2] =  x;
        minor_gridmarks[(4*i) + 3] = -_v;
    }

    float y = -_v;
    for (int i = width_minus_1; i < height_minus_1 + width_minus_1; i++) {
        y += _minor_grid_mark_distance;
        minor_gridmarks[4*i] =        _h;
        minor_gridmarks[(4*i) + 1] =  y;
        minor_gridmarks[(4*i) + 2] = -_h;
        minor_gridmarks[(4*i) + 3] =  y;
    }

    _minor_grid_indices_size = minor_grid_total_verts / 2;
    uint32_t minor_grid_indices[_minor_grid_indices_size];
    for (int i = 0; i < _minor_grid_indices_size; i++)
        minor_grid_indices[i] = i;

    x = -_h;
    for (int i = 0; i < (width_minus_1 / 10); i++) {
        x += major_grid_mark_distance;
        // four points, forming a line when drawn as a long rectangle
        major_gridmarks[8*i]       = x; // - normal
        major_gridmarks[(8*i) + 1] = _v;
        major_gridmarks[(8*i) + 2] = x; // - normal
        major_gridmarks[(8*i) + 3] = -_v;
        major_gridmarks[(8*i) + 4] = x; // + normal
        major_gridmarks[(8*i) + 5] = _v;
        major_gridmarks[(8*i) + 6] = x; // + normal
        major_gridmarks[(8*i) + 7] = -_v;
    }

    y = -_v;
    for (int i = (width_minus_1 / 10); i < (height_minus_1 / 10) + (width_minus_1 / 10); i++) {
        y += major_grid_mark_distance;
        major_gridmarks[8*i]       = _h;
        major_gridmarks[(8*i) + 1] = y;
        major_gridmarks[(8*i) + 2] = -_h;
        major_gridmarks[(8*i) + 3] = y;
        major_gridmarks[(8*i) + 4] = _h;
        major_gridmarks[(8*i) + 5] = y;
        major_gridmarks[(8*i) + 6] = -_h;
        major_gridmarks[(8*i) + 7] = y;
    }

    _major_grid_indices_size = (major_grid_total_verts / 8) * 6;
    uint32_t major_grid_indices[_major_grid_indices_size];
    int i = 0;
    for (int j = 0; j < major_grid_total_verts / 2; j+=4) {
        major_grid_indices[6*i]       = j + 0;
        major_grid_indices[(6*i) + 1] = j + 1;
        major_grid_indices[(6*i) + 2] = j + 2;
        major_grid_indices[(6*i) + 3] = j + 2;
        major_grid_indices[(6*i) + 4] = j + 1;
        major_grid_indices[(6*i) + 5] = j + 3;
        i++;
    };

    int major_gridmark_corner_size = major_grid_total_verts / 2;
    uint32_t major_gridmark_corner[major_gridmark_corner_size];
    for (int i = 0; i < major_grid_total_vertical_verts / 8; i++) {
        major_gridmark_corner[(4*i)]     = 0;
        major_gridmark_corner[(4*i) + 1] = 1;
        major_gridmark_corner[(4*i) + 2] = 2;
        major_gridmark_corner[(4*i) + 3] = 3;
    }
    for (int i = major_grid_total_vertical_verts / 8; i < major_gridmark_corner_size / 4; i++) {
        major_gridmark_corner[(4*i)]     = 4;
        major_gridmark_corner[(4*i) + 1] = 5;
        major_gridmark_corner[(4*i) + 2] = 6;
        major_gridmark_corner[(4*i) + 3] = 7;
    }

    float minor_color[4] = {0.5f, 0.5f, 0.5f, 1.f};
    float major_color[4] = {0.f, 0.f, 0.f, 1.f};

    if (minor_grid_total_verts > 0) {
        _minor_grid_shader->set_buffer("position", VariableType::Float32, {(size_t)(minor_grid_total_verts) / 2, 2}, minor_gridmarks);
        _minor_grid_shader->set_buffer("indices", VariableType::UInt32, {(size_t)_minor_grid_indices_size}, minor_grid_indices);
        _minor_grid_shader->set_buffer("colour", VariableType::Float32, {4}, minor_color);
    }

    if (major_grid_total_verts > 0) {
        _major_grid_shader->set_buffer("position", VariableType::Float32, {(size_t)(major_grid_total_verts) / 2, 2}, major_gridmarks);
        _major_grid_shader->set_buffer("indices", VariableType::UInt32, {(size_t)_major_grid_indices_size}, major_grid_indices);
        _major_grid_shader->set_buffer("corner", VariableType::UInt32, {(size_t)major_gridmark_corner_size}, major_gridmark_corner);
        _major_grid_shader->set_buffer("colour", VariableType::Float32, {4}, major_color);
    }

    update_backstitch_buffers();
    _drawing = true;
}

const u_int32_t circle_vert_layout[50 * 3] = {
    0, 1, 2,    0, 2, 3,    0, 3, 4,    0, 4, 5,
    0, 5, 6,    0, 6, 7,    0, 7, 8,    0, 8, 9,
    0, 9, 10,   0, 10, 11,  0, 11, 12,  0, 12, 13,
    0, 13, 14,  0, 14, 15,  0, 15, 16,  0, 16, 17,
    0, 17, 18,  0, 18, 19,  0, 19, 20,  0, 20, 21,
    0, 21, 22,  0, 22, 23,  0, 23, 24,  0, 24, 25,
    0, 25, 26,  0, 26, 27,  0, 27, 28,  0, 28, 29,
    0, 29, 30,  0, 30, 31,  0, 31, 32,  0, 32, 33,
    0, 33, 34,  0, 34, 35,  0, 35, 36,  0, 36, 37,
    0, 37, 38,  0, 38, 39,  0, 39, 40,  0, 40, 41,
    0, 41, 42,  0, 42, 43,  0, 43, 44,  0, 44, 45,
    0, 45, 46,  0, 46, 47,  0, 47, 48,  0, 48, 49,
    0, 49, 50,  0, 50, 1
};

void create_circle_vertices(Vector2f center, float r, int num_segments, std::vector<float> *pos_buffer, std::vector<uid_t> *ind_buffer) {
    uint32_t circle_start = pos_buffer->size() == 0 ? 0 : pos_buffer->size() / 2;

    float theta = 3.1415926 * 2 / float(num_segments);
    float tangetial_factor = tanf(theta);
    float radial_factor = cosf(theta);

    float x = r;
    float y = 0;

    // center vertex
    pos_buffer->push_back(center[0]);
    pos_buffer->push_back(center[1]);

    for (int i = 0; i < num_segments; i++) {
        pos_buffer->push_back(x + center[0]);
        pos_buffer->push_back(y + center[1]);

        // calculate the tangential vector
        // remember, the radial vector is (x, y)
        // to get the tangential vector we flip those coordinates and negate one of them
        float tx = -y;
        float ty = x;

        // add the tangential vector
        x += tx * tangetial_factor;
        y += ty * tangetial_factor;

        // correct using the radial factor
        x *= radial_factor;
        y *= radial_factor;
    }

    for (u_int32_t indices : circle_vert_layout)
        ind_buffer->push_back(indices + circle_start);
}

void create_line_vertices(Vector2f start, Vector2f end, Vector2f normal_vector, std::vector<float> *pos_buffer, std::vector<uint32_t> *ind_buffer) {
    uint32_t line_start = pos_buffer->size() == 0 ? 0 : pos_buffer->size() / 2;

    pos_buffer->push_back(start[0] + normal_vector[0]);
    pos_buffer->push_back(start[1] + normal_vector[1]);
    pos_buffer->push_back(start[0] - normal_vector[0]);
    pos_buffer->push_back(start[1] - normal_vector[1]);
    pos_buffer->push_back(end[0] + normal_vector[0]);
    pos_buffer->push_back(end[1] + normal_vector[1]);
    pos_buffer->push_back(end[0] - normal_vector[0]);
    pos_buffer->push_back(end[1] - normal_vector[1]);

    ind_buffer->push_back(line_start + 2);
    ind_buffer->push_back(line_start + 1);
    ind_buffer->push_back(line_start + 0);
    ind_buffer->push_back(line_start + 3);
    ind_buffer->push_back(line_start + 1);
    ind_buffer->push_back(line_start + 2);
}

void CanvasRenderer::update_backstitch_buffers() {
    std::vector<BackStitch> *backstitches = &_app->_project->backstitches;
    int no_backstitches = backstitches->size();
    int no_circle_verts = 50;
    float normal = _minor_grid_mark_distance * 0.125f;
    int total_verts_per_segment = no_circle_verts + no_circle_verts + 2 + 4; // two circles and a line

    std::vector<float> backstitch_positions;
    std::vector<float> backstitch_colours;
    std::vector<u_int32_t> backstitch_indices;

    for (BackStitch bs : *backstitches) {
        Vector2f start_ndc = (bs.start * _minor_grid_mark_distance) - Vector2f(_h, _v);
        Vector2f end_ndc = (bs.end * _minor_grid_mark_distance) - Vector2f(_h, _v);
        Vector2f line_vector = bs.end - bs.start;
        Vector2f perpendicular_vector = normalize(Vector2f(line_vector[1], -line_vector[0]));
        Vector2f normal_vector = normal * perpendicular_vector;

        create_circle_vertices(start_ndc, normal, no_circle_verts, &backstitch_positions, &backstitch_indices);
        create_line_vertices(start_ndc, end_ndc, normal_vector, &backstitch_positions, &backstitch_indices);
        create_circle_vertices(end_ndc, normal, no_circle_verts, &backstitch_positions, &backstitch_indices);

        // colour for all vertices
        Thread *t = _app->_project->palette[bs.palette_index];
        Color c = t->color();
        for (int i = 0; i < total_verts_per_segment; i++) {
            backstitch_colours.push_back(c.r());
            backstitch_colours.push_back(c.g());
            backstitch_colours.push_back(c.b());
            backstitch_colours.push_back(c.a());
        }
    }

    _backstitch_indices_size = backstitch_indices.size();

    if (no_backstitches > 0) {
        _back_stitch_shader->set_buffer("position", VariableType::Float32, {backstitch_positions.size() / 2, 2}, backstitch_positions.data());
        _back_stitch_shader->set_buffer("indices", VariableType::UInt32, {(size_t)_backstitch_indices_size}, backstitch_indices.data());
        _back_stitch_shader->set_buffer("colour", VariableType::Float32, {backstitch_colours.size()}, backstitch_colours.data());
    }
}

void CanvasRenderer::clear_ghost_backstitch() {
    _backstitch_ghost_indices_size = 0;
}

void CanvasRenderer::move_ghost_backstitch(Vector2f end, Thread *thread) {
    Vector2f start = _app->_previous_backstitch_point;
    if (end == NO_SUBSTITCH_SELECTED || start == NO_SUBSTITCH_SELECTED) {
        _backstitch_ghost_indices_size = 0;
        return;
    }

    int no_circle_verts = 50;
    int total_verts_per_segment = no_circle_verts + no_circle_verts + 2 + 4; // two circles and a line
    float normal = _minor_grid_mark_distance * 0.125f;

    std::vector<float> backstitch_positions;
    std::vector<float> backstitch_colours;
    std::vector<u_int32_t> backstitch_indices;

    Vector2f start_ndc = (start * _minor_grid_mark_distance) - Vector2f(_h, _v);
    Vector2f end_ndc = (end * _minor_grid_mark_distance) - Vector2f(_h, _v);
    Vector2f line_vector = end - start;
    Vector2f perpendicular_vector = normalize(Vector2f(line_vector[1], -line_vector[0]));
    Vector2f normal_vector = normal * perpendicular_vector;

    create_circle_vertices(start_ndc, normal, no_circle_verts, &backstitch_positions, &backstitch_indices);
    create_line_vertices(start_ndc, end_ndc, normal_vector, &backstitch_positions, &backstitch_indices);
    create_circle_vertices(end_ndc, normal, no_circle_verts, &backstitch_positions, &backstitch_indices);

    // colour for all vertices
    Color c = thread->color();
    for (int i = 0; i < total_verts_per_segment; i++) {
        backstitch_colours.push_back(c.r());
        backstitch_colours.push_back(c.g());
        backstitch_colours.push_back(c.b());
        backstitch_colours.push_back(c.a());
    }

    _backstitch_ghost_indices_size = backstitch_indices.size();

    _back_stitch_ghost_shader->set_buffer("position", VariableType::Float32, {backstitch_positions.size() / 2, 2}, backstitch_positions.data());
    _back_stitch_ghost_shader->set_buffer("indices", VariableType::UInt32, {(size_t)_backstitch_ghost_indices_size}, backstitch_indices.data());
    _back_stitch_ghost_shader->set_buffer("colour", VariableType::Float32, {backstitch_colours.size()}, backstitch_colours.data());
}

Vector2i CanvasRenderer::get_mouse_position() {
    Vector4f bounds = _camera->canvas_bounds(_position);
    try {
        Vector2f mouse_ndc = _camera->screen_to_ndc(_app->mouse_pos());
        return _camera->ndc_to_stitch(mouse_ndc, bounds);
    } catch (std::invalid_argument&) {
        return NO_STITCH_SELECTED;
    }
}

Vector2f CanvasRenderer::get_mouse_subposition() {
    Vector4f bounds = _camera->canvas_bounds(_position);
    try {
        Vector2f mouse_ndc = _camera->screen_to_ndc(_app->mouse_pos());
        return _camera->ndc_to_substitch(mouse_ndc, bounds);
    } catch (std::invalid_argument&) {
        return NO_SUBSTITCH_SELECTED;
    }
}

void CanvasRenderer::upload_texture() {
    _texture->upload(_app->_project->texture_data_array.get());
}

void CanvasRenderer::render() {
    if (!_drawing)
        return;

    Vector2i device_size = _app->framebuffer_size();
    _render_pass->resize(device_size);

    _selected_stitch = get_mouse_position();
    _selected_sub_stitch = get_mouse_subposition();

    if (_backstitch_ghost_indices_size != 0 && _selected_sub_stitch == NO_SUBSTITCH_SELECTED || _app->_selected_tool != ToolOptions::BACK_STITCH)
        clear_ghost_backstitch();

    float pixel_canvas_height = _canvas_height_ndc * _camera->get_scale() * (float)(device_size[1]) / _app->pixel_ratio();
    float minor_grid_mark_pixel_distance = pixel_canvas_height * _minor_grid_mark_distance;

    Matrix4f mvp = _camera->generate_matrices();

    _render_pass->begin();

    // 20.f is a better value, but the gridmark stretching is more apparent when in fullscreen
    render_cs_shader(mvp);
    if (minor_grid_mark_pixel_distance >= 30.f) {
        render_minor_grid_shader(mvp);
        render_major_grid_shader(mvp);
    }
    render_back_stitch_shader(mvp);
    render_back_stitch_ghost_shader(mvp);

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
    if (_minor_grid_shader == nullptr || _minor_grid_indices_size == 0)
        return;

    _minor_grid_shader->set_uniform("mvp", mvp);

    _minor_grid_shader->begin();
    _minor_grid_shader->draw_array(Shader::PrimitiveType::Line, 0,
                                   _minor_grid_indices_size, true);
    _minor_grid_shader->end();
}

void CanvasRenderer::render_major_grid_shader(Matrix4f mvp) {
    if (_major_grid_shader == nullptr || _major_grid_indices_size == 0)
        return;

    // find distance in ortho ndc of a pixel
    Vector2f p1 = _camera->screen_to_ndc(Vector2i(0, 0));
    Vector2f p2 = _camera->screen_to_ndc(Vector2i(0, 1));
    float normal = p2[1] - p1[1];

    _major_grid_shader->set_uniform("normal", normal);
    _major_grid_shader->set_uniform("mvp", mvp);

    _major_grid_shader->begin();
    _major_grid_shader->draw_array(Shader::PrimitiveType::Triangle, 0,
                                   _major_grid_indices_size, true);
    _major_grid_shader->end();
}

void CanvasRenderer::render_back_stitch_shader(Matrix4f mvp) {
    if (_back_stitch_shader == nullptr || _backstitch_indices_size == 0)
        return;

    _back_stitch_shader->set_uniform("mvp", mvp);

    _back_stitch_shader->begin();
    _back_stitch_shader->draw_array(Shader::PrimitiveType::Triangle, 0,
                                    _backstitch_indices_size, true);
    _back_stitch_shader->end();
}

void CanvasRenderer::render_back_stitch_ghost_shader(Matrix4f mvp) {
    if (_back_stitch_ghost_shader == nullptr || _backstitch_ghost_indices_size == 0)
        return;

    _back_stitch_ghost_shader->set_uniform("mvp", mvp);

    _back_stitch_ghost_shader->begin();
    _back_stitch_ghost_shader->draw_array(Shader::PrimitiveType::Triangle, 0,
                                          _backstitch_ghost_indices_size, true);
    _back_stitch_ghost_shader->end();
}