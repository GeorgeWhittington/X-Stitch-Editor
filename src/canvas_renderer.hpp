#pragma once
#include <memory>
#include <nanogui/nanogui.h>
#include "threads.hpp"

using nanogui::Vector2i;

class XStitchEditorApplication;
class Camera2D;

const Vector2i NO_STITCH_SELECTED = Vector2i(-1, -1);

class CanvasRenderer {
public:
    CanvasRenderer(XStitchEditorApplication *app);
    void draw_to_canvas(Vector2i stitch, Thread *thread);
    void erase_from_canvas(Vector2i stitch);
    void render();

    Camera2D *_camera;
    float _position[3*4];
    Vector2i _selected_stitch = NO_STITCH_SELECTED;

private:
    Vector2i get_mouse_position();
    void render_cs_shader(nanogui::Matrix4f mvp);
    void render_minor_grid_shader(nanogui::Matrix4f mvp);
    void render_major_grid_shader(nanogui::Matrix4f mvp);

    XStitchEditorApplication *_app;

    nanogui::RenderPass *_render_pass;
    nanogui::Shader *_cross_stitch_shader;
    nanogui::Shader *_minor_grid_shader;
    nanogui::Shader *_major_grid_shader;
    nanogui::Texture *_texture;

    // std::unique_ptr _thread_data;
    std::unique_ptr<uint8_t[]> _texture_data_array;

    float _canvas_height_ndc;
    float _minor_grid_mark_distance;
    int _minor_grid_total_verts;
    int _major_grid_indices_size;
};

// Shader kernels
// TODO: test OPENGL and GLES kernels
#if defined(NANOGUI_USE_OPENGL)
const std::string GRID_VERT = R"(
    #version 330 core
    layout (location = 0) in vec2 position;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(position, 0.0, 1.0);
    }
)";
const std::string GRID_FRAG = R"(
    #version 330 core
    out vec4 FragColor;

    uniform vec4 colour;

    void main() {
        FragColor = colour;
    }
)";

const std::string CROSS_STITCH_VERT = R"(
    #version 330 core
    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 tex;

    out vec2 textureCoord;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(position, 1.0);
        textureCoord = tex;
    }
)";
const std::string CROSS_STITCH_FRAG = R"(
    #version 330 core
    in vec2 textureCoord;
    out vec4 FragColor;

    uniform sampler2D cross_stitch_texture;

    void main() {
        FragColor = texture(cross_stitch_texture, textureCoord);
    }
)";
#elif defined(NANOGUI_USE_GLES)
const std::string GRID_VERT = R"(
    precision highp float;
    attribute vec3 position;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(position, 1.0);
    }
)";
const std::string GRID_FRAG = R"(
    precision highp float;

    uniform vec4 colour;

    void main() {
        gl_FragColor = colour;
    }
)";

const std::string CROSS_STITCH_VERT = R"(
    precision highp float;
    attribute vec3 position;
    attribute vec2 tex;

    varying vec2 textureCoord;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(position, 1.0);
        textureCoord = tex;
    }
)";
const std::string CROSS_STITCH_FRAG = R"(
    precision highp float;
    varying vec2 textureCoord;

    uniform sampler2D cross_stitch_texture;

    void main() {
        gl_FragColor = texture(cross_stitch_texture, textureCoord);
    }
)";
#elif defined(NANOGUI_USE_METAL)
const std::string GRID_VERT = R"(
    using namespace metal;
    struct VertexOut {
        float4 position [[position]];
    };

    vertex VertexOut vertex_main(const device packed_float2 *position,
                                 constant float4x4 &mvp,
                                 uint id [[vertex_id]]) {
        VertexOut vert;
        vert.position = mvp * float4(position[id], 0.f, 1.f);
        return vert;
    }
)";
const std::string GRID_FRAG = R"(
    using namespace metal;

    struct VertexOut {
        float4 position [[position]];
    };

    fragment float4 fragment_main(VertexOut vert [[stage_in]],
                                  constant float4 &colour) {
        return colour;
    }
)";

const std::string CROSS_STITCH_VERT = R"(
    using namespace metal;
    struct VertexOut {
        float4 position [[position]];
        float2 tex;
    };

    vertex VertexOut vertex_main(const device packed_float3 *position,
                                 const device float2 *tex,
                                 constant float4x4 &mvp,
                                 uint id [[vertex_id]]) {
        VertexOut vert;
        vert.position = mvp * float4(position[id], 1.f);
        vert.tex = tex[id];
        return vert;
    }
)";
const std::string CROSS_STITCH_FRAG = R"(
    using namespace metal;

    struct VertexOut {
        float4 position [[position]];
        float2 tex;
    };

    fragment float4 fragment_main(VertexOut vert [[stage_in]],
                                  texture2d<float, access::sample> cross_stitch_texture,
                                  sampler cross_stitch_sampler) {
        return cross_stitch_texture.sample(cross_stitch_sampler, vert.tex);
    }
)";
#endif