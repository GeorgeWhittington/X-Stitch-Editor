#pragma once
#include <nanogui/nanogui.h>
#include <memory>
#include "threads.hpp"
#include "constants.hpp"

class XStitchEditorApplication;
class Camera2D;

class CanvasRenderer {
public:
    CanvasRenderer(XStitchEditorApplication *app);
    void update_backstitch_buffers();
    void clear_ghost_backstitch();
    void move_ghost_backstitch(nanogui::Vector2f end, Thread *thread);
    void upload_texture();
    void render();

    std::shared_ptr<Camera2D> _camera;
    float _position[3*4];
    nanogui::Vector2i _selected_stitch = NO_STITCH_SELECTED;
    nanogui::Vector2f _selected_sub_stitch = NO_SUBSTITCH_SELECTED;

private:
    nanogui::Vector2i get_mouse_position();
    nanogui::Vector2f get_mouse_subposition();
    void render_cs_shader(nanogui::Matrix4f mvp);
    void render_minor_grid_shader(nanogui::Matrix4f mvp);
    void render_major_grid_shader(nanogui::Matrix4f mvp);
    void render_back_stitch_shader(nanogui::Matrix4f mvp);
    void render_back_stitch_ghost_shader(nanogui::Matrix4f mvp);

    XStitchEditorApplication *_app;

    std::unique_ptr<nanogui::RenderPass> _render_pass;
    std::unique_ptr<nanogui::Shader> _cross_stitch_shader;
    std::unique_ptr<nanogui::Shader> _minor_grid_shader;
    std::unique_ptr<nanogui::Shader> _major_grid_shader;
    std::unique_ptr<nanogui::Shader> _back_stitch_shader;
    std::unique_ptr<nanogui::Shader> _back_stitch_ghost_shader;
    std::unique_ptr<nanogui::Texture> _texture;

    float _canvas_height_ndc;
    float _minor_grid_mark_distance;
    int _major_grid_indices_size;
    int _minor_grid_indices_size;
    int _backstitch_indices_size = 0;
    int _backstitch_ghost_indices_size = 0;
    float _h = 1.f;
    float _v = 1.f;
};

// Shader kernels
// TODO: test OPENGL and GLES kernels
#if defined(NANOGUI_USE_OPENGL)
const std::string BACK_STITCH_VERT = R"(
    #version 330 core
    layout (location = 0) in vec2 position;
    layout (location = 1) in vec4 colour;

    out vec4 frag_colour;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(position, 0.0, 1.0);
        frag_colour = colour;
    }
)";
const std::string BACK_STITCH_FRAG = R"(
    #version 330 core
    in vec4 frag_colour;
    out vec4 FragColor;

    void main() {
        FragColor = frag_colour;
    }
)";

const std::string MINOR_GRID_VERT = R"(
    #version 330 core
    layout (location = 0) in vec2 position;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(position, 0.0, 1.0);
    }
)";
const std::string MINOR_GRID_FRAG = R"(
    #version 330 core
    out vec4 FragColor;

    uniform vec4 colour;

    void main() {
        FragColor = colour;
    }
)";

const std::string MAJOR_GRID_VERT = R"(
    #version 330 core
    layout (location = 0) in vec2 position;
    layout (location = 1) in uint8_t corner;

    uniform mat4 mvp;
    uniform float normal;

    void main() {
        vec4 pos = mvp * vec4(position, 0.0, 1.0);

        if (corner == 0 || corner == 1) {
            pos = vec4(pos.x - normal, pos.yzw);
        } else if (corner == 2 || corner == 3) {
            pos = vec4(pos.x + normal, pos.yzw);
        } else if (corner == 4 || corner == 5) {
            pos = vec4(pos.x, pos.y - normal, pos.zw);
        } else if (corner == 6 || corner == 7) {
            pos = vec4(pos.x, pos.y + normal, pos.zw);
        }

        gl_Position = pos;
    }
)";
const std::string MAJOR_GRID_FRAG = R"(
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
const std::string BACK_STITCH_VERT = R"(
    precision highp float;
    attribute vec2 position;
    attribute vec4 colour;

    varying vec4 frag_colour;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(position, 0.0, 1.0);
        frag_colour = colour;
    }
)";
const std::string BACK_STITCH_FRAG = R"(
    precision highp float;
    varying vec4 frag_colour;

    void main() {
        FragColor = frag_colour;
    }
)";

const std::string MINOR_GRID_VERT = R"(
    precision highp float;
    attribute vec2 position;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(position, 0.0, 1.0);
    }
)";
const std::string MINOR_GRID_FRAG = R"(
    precision highp float;
    uniform vec4 colour;

    void main() {
        gl_FragColor = colour;
    }
)";

const std::string MAJOR_GRID_VERT = R"(
    precision highp float;
    attribute vec2 position;
    attribute uint8_t corner;

    uniform float normal;
    uniform mat4 mvp;

    void main() {
        vec4 pos = mvp * vec4(position, 0.0, 1.0);

        if (corner == 0 || corner == 1) {
            pos = vec4(pos.x - normal, pos.yzw);
        } else if (corner == 2 || corner == 3) {
            pos = vec4(pos.x + normal, pos.yzw);
        } else if (corner == 4 || corner == 5) {
            pos = vec4(pos.x, pos.y - normal, pos.zw);
        } else if (corner == 6 || corner == 7) {
            pos = vec4(pos.x, pos.y + normal, pos.zw);
        }

        gl_Position = pos;
    }
)";
const std::string MAJOR_GRID_FRAG = R"(
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
const std::string BACK_STITCH_VERT = R"(
    using namespace metal;
    struct VertexOut {
        float4 position [[position]];
        float4 frag_colour;
    };

    vertex VertexOut vertex_main(const device packed_float2 *position,
                                 const device float4 *colour,
                                 constant float4x4 &mvp,
                                 uint id [[vertex_id]]) {
        VertexOut vert;
        vert.position = mvp * float4(position[id], 0.f, 1.f);
        vert.frag_colour = colour[id];
        return vert;
    }
)";
const std::string BACK_STITCH_FRAG = R"(
    using namespace metal;

    struct VertexOut {
        float4 position [[position]];
        float4 frag_colour;
    };

    fragment float4 fragment_main(VertexOut vert [[stage_in]]) {
        return vert.frag_colour;
    }
)";

const std::string MINOR_GRID_VERT = R"(
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
const std::string MINOR_GRID_FRAG = R"(
    using namespace metal;

    struct VertexOut {
        float4 position [[position]];
    };

    fragment float4 fragment_main(VertexOut vert [[stage_in]],
                                  constant float4 &colour) {
        return colour;
    }
)";

const std::string MAJOR_GRID_VERT = R"(
    using namespace metal;
    struct VertexOut {
        float4 position [[position]];
    };

    vertex VertexOut vertex_main(const device packed_float2 *position,
                                 const device uint8_t *corner,
                                 constant float &normal,
                                 constant float4x4 &mvp,
                                 uint id [[vertex_id]]) {
        float4 pos = mvp * float4(position[id], 0.f, 1.f);

        if (corner[id] == 0 || corner[id] == 1) {
            pos = float4(pos.x - normal, pos.yzw);
        } else if (corner[id] == 2 || corner[id] == 3) {
            pos = float4(pos.x + normal, pos.yzw);
        } else if (corner[id] == 4 || corner[id] == 5) {
            pos = float4(pos.x, pos.y - normal, pos.zw);
        } else if (corner[id] == 6 || corner[id] == 7) {
            pos = float4(pos.x, pos.y + normal, pos.zw);
        }

        VertexOut vert;
        vert.position = pos;
        return vert;
    }
)";
// TODO: AntiAliasing!!
const std::string MAJOR_GRID_FRAG = R"(
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