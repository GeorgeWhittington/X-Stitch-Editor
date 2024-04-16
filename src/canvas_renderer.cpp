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
    _back_stitch_shader = new Shader(_render_pass, "back_stitch", BACK_STITCH_VERT, BACK_STITCH_FRAG);
    _back_stitch_cap_shader = new Shader(_render_pass, "back_stitch_cap", BACK_STITCH_CAP_VERT, BACK_STITCH_CAP_FRAG);

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
    _cross_stitch_shader->set_texture("cross_stitch_texture", _texture);

    _minor_grid_mark_distance = 2.f / (float)std::max(width, height); // ndc space is 2.f long in the axis that is largest
    float major_grid_mark_distance = _minor_grid_mark_distance * 10.f;
    float major_grid_mark_normal = _minor_grid_mark_distance * 0.125f;

    int width_minus_1 = width - 1;
    int height_minus_1 = height - 1;

    _minor_grid_total_verts = (height_minus_1 * 4) + (width_minus_1 * 4);
    int major_grid_total_verts = ((height_minus_1 / 10) * 8) + ((width_minus_1 / 10) * 8);
    float minor_gridmarks[_minor_grid_total_verts];
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

    x = -_h;
    for (int i = 0; i < (width_minus_1 / 10); i++) {
        x += major_grid_mark_distance;
        // four points, forming a line when drawn as a long rectangle
        major_gridmarks[8*i]       = x - major_grid_mark_normal;
        major_gridmarks[(8*i) + 1] = _v;
        major_gridmarks[(8*i) + 2] = x - major_grid_mark_normal;
        major_gridmarks[(8*i) + 3] = -_v;
        major_gridmarks[(8*i) + 4] = x + major_grid_mark_normal;
        major_gridmarks[(8*i) + 5] = _v;
        major_gridmarks[(8*i) + 6] = x + major_grid_mark_normal;
        major_gridmarks[(8*i) + 7] = -_v;
    }

    y = -_v;
    for (int i = (width_minus_1 / 10); i < (height_minus_1 / 10) + (width_minus_1 / 10); i++) {
        y += major_grid_mark_distance;
        major_gridmarks[8*i]       = _h;
        major_gridmarks[(8*i) + 1] = y - major_grid_mark_normal;
        major_gridmarks[(8*i) + 2] = -_h;
        major_gridmarks[(8*i) + 3] = y - major_grid_mark_normal;
        major_gridmarks[(8*i) + 4] = _h;
        major_gridmarks[(8*i) + 5] = y + major_grid_mark_normal;
        major_gridmarks[(8*i) + 6] = -_h;
        major_gridmarks[(8*i) + 7] = y + major_grid_mark_normal;
    }

    _major_grid_indices_size = (major_grid_total_verts / 4) * 6;
    uint32_t major_grid_indices[_major_grid_indices_size];
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
        _major_grid_shader->set_buffer("indices", VariableType::UInt32, {(size_t)_major_grid_indices_size}, major_grid_indices);
        _major_grid_shader->set_buffer("colour", VariableType::Float32, {4}, major_color);
    } else {
        delete _major_grid_shader;
        _major_grid_shader = nullptr;
    }

    // _back_stitch_cap_shader->set_uniform("radius", major_grid_mark_normal);
};

void create_circle_vertices(float cx, float cy, float r, int num_segments, std::vector<float> *buffer) {
    float theta = 3.1415926 * 2 / float(num_segments);
    float tangetial_factor = tanf(theta);
    float radial_factor = cosf(theta);

    float x = r;
    float y = 0;

    // center vertex
    buffer->push_back(cx);
    buffer->push_back(cy);

    for (int i = 0; i < num_segments; i++)
    {
        buffer->push_back(x + cx);
        buffer->push_back(y + cy);

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
}

void CanvasRenderer::update_backstitch_buffers() {
    std::vector<BackStitch> *backstitches = &_app->_project->backstitches;
    int no_backstitches = backstitches->size();

    // TODO: will this look fine at all zoom levels?
    // may need to consider geometry shaders if it won't
    // Or just send 2x start point + 2x end point for each line and
    // calculate + offset those points inside the vertex shader using data passed in?
    float normal = _minor_grid_mark_distance * 0.125f;

    float backstitch_positions[8 * no_backstitches]; // 4 positions per bs, each position takes 2 values
    float backstitch_colours[4 * 4 * no_backstitches]; // 4 channel colour for each position

    for (int i = 0; i < no_backstitches; i++) {
        BackStitch bs = backstitches->at(i);
        Thread *t = _app->_project->palette[bs.palette_index];
        Color c = t->color();

        Vector2f line_vector = bs.end - bs.start;
        Vector2f perpendicular_vector = normalize(Vector2f(line_vector[1], -line_vector[0]));

        Vector2f start_ndc = (bs.start * _minor_grid_mark_distance) - Vector2f(_h, _v);
        Vector2f end_ndc = (bs.end * _minor_grid_mark_distance) - Vector2f(_h, _v);
        Vector2f normal_vector = normal * perpendicular_vector;

        backstitch_positions[(8*i)]     = start_ndc[0] + normal_vector[0];
        backstitch_positions[(8*i) + 1] = start_ndc[1] + normal_vector[1];
        backstitch_positions[(8*i) + 2] = start_ndc[0] - normal_vector[0];
        backstitch_positions[(8*i) + 3] = start_ndc[1] - normal_vector[1];
        backstitch_positions[(8*i) + 4] = end_ndc[0] + normal_vector[0];
        backstitch_positions[(8*i) + 5] = end_ndc[1] + normal_vector[1];
        backstitch_positions[(8*i) + 6] = end_ndc[0] - normal_vector[0];
        backstitch_positions[(8*i) + 7] = end_ndc[1] - normal_vector[1];

        for (int j = 0; j < 4; j++) {
            backstitch_colours[(16*i) + 0 + (4*j)] = c.r();
            backstitch_colours[(16*i) + 1 + (4*j)] = c.g();
            backstitch_colours[(16*i) + 2 + (4*j)] = c.b();
            backstitch_colours[(16*i) + 3 + (4*j)] = c.a();
        }
    }

    _backstitch_indices_size = 6 * no_backstitches;
    uint32_t backstitch_indices[_backstitch_indices_size];
    for (int i = 0; i < no_backstitches; i++) {
        backstitch_indices[6*i]       = (i*4) + 2;
        backstitch_indices[(6*i) + 1] = (i*4) + 1;
        backstitch_indices[(6*i) + 2] = (i*4) + 0;
        backstitch_indices[(6*i) + 3] = (i*4) + 3;
        backstitch_indices[(6*i) + 4] = (i*4) + 1;
        backstitch_indices[(6*i) + 5] = (i*4) + 2;
    }

    if (no_backstitches > 0) {
        _back_stitch_shader->set_buffer("position", VariableType::Float32, {(size_t)no_backstitches * 4, 2}, backstitch_positions);
        _back_stitch_shader->set_buffer("indices", VariableType::UInt32, {(size_t)_backstitch_indices_size}, backstitch_indices);
        _back_stitch_shader->set_buffer("colour", VariableType::Float32, {(size_t)no_backstitches * 4 * 4}, backstitch_colours);
    }

    // Find all unique backstitch end points. Override colour
    // if a lower palette index can be found for a point, because
    // line segments are drawn with lower palette indexes last
    std::vector<std::pair<Vector2f, int>> backstitch_caps;
    for (const BackStitch bs : *backstitches) {
        Vector2f points[2] = {bs.start, bs.end};

        for (const Vector2f point : points) {
            bool already_contained = false;
            for (auto itr = backstitch_caps.begin(); itr < backstitch_caps.end(); itr++) {
                if ((*itr).first == point) {
                    already_contained = true;
                    if ((*itr).second > bs.palette_index)
                        (*itr).second = bs.palette_index;
                    break;
                }
            }
            if (!already_contained)
                backstitch_caps.push_back({point, bs.palette_index});
        }
    }

    // construct positions for one circle that is centered on (-h, -v)
    // provide indices and repeat indices for n.o. circles I wanna make
    // provide transformation buffer that describes how each circle should be adjusted? (coords * width of one stitch in ndc)

    int no_backstitch_caps = backstitch_caps.size();
    int no_circle_verts = 50;

    int positions_per_cap = no_circle_verts + 1;
    int backstitch_cap_positions_size = positions_per_cap * no_backstitch_caps * 2;
    std::vector<float> backstitch_cap_positions;
    backstitch_cap_positions.reserve(backstitch_cap_positions_size);

    _backstitch_cap_indices_size = no_circle_verts * 3 * no_backstitch_caps;
    uint32_t backstitch_cap_indices[_backstitch_cap_indices_size];

    int backstitch_cap_colours_size = positions_per_cap * no_backstitch_caps * 4;
    std::vector<float> backstitch_cap_colours;
    backstitch_cap_colours.reserve(backstitch_cap_colours_size);

    for (int i = 0; i < no_backstitch_caps; i++) {
        Vector2f center = backstitch_caps[i].first;
        Vector2f center_ndc = (center * _minor_grid_mark_distance) - Vector2f(_h, _v);

        create_circle_vertices(center_ndc[0], center_ndc[1], normal, no_circle_verts, &backstitch_cap_positions);

        int stride = no_circle_verts * 3;
        for (int j = 0; j < no_circle_verts; j++) {
            backstitch_cap_indices[(stride*i) + (3*j)]     = (i * positions_per_cap);
            backstitch_cap_indices[(stride*i) + (3*j) + 1] = (i * positions_per_cap) + j + 1;
            backstitch_cap_indices[(stride*i) + (3*j) + 2] = (i * positions_per_cap) + j + 2;

            if (j == no_circle_verts - 1)
                backstitch_cap_indices[(stride*i) + (3*j) + 2] = (i * positions_per_cap) + 1;
        }

        Thread *t = _app->_project->palette[backstitch_caps[i].second];
        float r = t->color().r();
        float g = t->color().g();
        float b = t->color().b();

        for (int j = 0; j < positions_per_cap; j++) {
            backstitch_cap_colours.push_back(r);
            backstitch_cap_colours.push_back(g);
            backstitch_cap_colours.push_back(b);
            backstitch_cap_colours.push_back(1.f);
        }
    }

    if (no_backstitch_caps > 0) {
        _back_stitch_cap_shader->set_buffer("position", VariableType::Float32, {(size_t)positions_per_cap * no_backstitch_caps, 2}, backstitch_cap_positions.data());
        _back_stitch_cap_shader->set_buffer("indices", VariableType::UInt32, {(size_t)_backstitch_cap_indices_size}, backstitch_cap_indices);
        _back_stitch_cap_shader->set_buffer("colour", VariableType::Float32, {(size_t)backstitch_cap_colours_size}, backstitch_cap_colours.data());
    }
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
    Vector2i device_size = _app->framebuffer_size();
    _render_pass->resize(device_size);

    _selected_stitch = get_mouse_position();
    _selected_sub_stitch = get_mouse_subposition();

    float pixel_canvas_height = _canvas_height_ndc * _camera->get_scale() * (float)(device_size[1]) / _app->pixel_ratio();
    float minor_grid_mark_pixel_distance = pixel_canvas_height * _minor_grid_mark_distance;

    Matrix4f mvp = _camera->generate_matrices();

    _render_pass->begin();

    render_cs_shader(mvp);
    if (minor_grid_mark_pixel_distance >= 15.f) {
        render_minor_grid_shader(mvp);
        render_major_grid_shader(mvp);
    }
    render_back_stitch_shader(mvp);
    render_back_stitch_cap_shader(mvp);

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

void CanvasRenderer::render_back_stitch_shader(Matrix4f mvp) {
    if (_back_stitch_shader == nullptr || _backstitch_indices_size == 0)
        return;

    _back_stitch_shader->set_uniform("mvp", mvp);

    _back_stitch_shader->begin();
    _back_stitch_shader->draw_array(Shader::PrimitiveType::Triangle, 0,
                                    _backstitch_indices_size, true);
    _back_stitch_shader->end();
}

void CanvasRenderer::render_back_stitch_cap_shader(Matrix4f mvp) {
    if (_back_stitch_cap_shader == nullptr || _backstitch_cap_indices_size == 0)
        return;

    _back_stitch_cap_shader->set_uniform("mvp", mvp);

    _back_stitch_cap_shader->begin();
    _back_stitch_cap_shader->draw_array(Shader::PrimitiveType::Triangle, 0,
                                        _backstitch_cap_indices_size, true);
    _back_stitch_cap_shader->end();
}