from enum import Enum

import nanogui
import numpy as np
import glm

from app import GLOBAL_THREAD_LOOKUP, Camera2D

RenderingAPI = Enum("RenderingAPI", ["OPENGL", "GLES", "METAL"])

GRID_VERT = {
    RenderingAPI.OPENGL: """
    #version 330 core
    layout (location = 0) in vec2 position;

    void main() {
        gl_Position = mvp * vec4(position, 0.0, 1.0);
    }
    """,
    RenderingAPI.GLES: """
    """,
    RenderingAPI.METAL: """
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
    }"""
}

GRID_FRAG = {
    RenderingAPI.OPENGL: """
    #version 330 core
    out vec4 FragColor;

    uniform vec4 colour;

    void main() {
        FragColor = colour;
    }""",
    RenderingAPI.GLES: """
    """,
    RenderingAPI.METAL: """
    using namespace metal;

    struct VertexOut {
        float4 position [[position]];
    };

    fragment float4 fragment_main(VertexOut vert [[stage_in]],
                                  constant float4 &colour) {
        return colour;
    }"""
}

X_STITCH_VERT = {
    RenderingAPI.OPENGL: """
    #version 330 core
    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 tex;

    out vec2 textureCoord;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(position, 1.0);
        textureCoord = tex;
    }""",
    RenderingAPI.GLES: """
    """,
    RenderingAPI.METAL: """
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
    }"""
}

# TODO: colour transparent pixels to a user-specified colour, so that users can design things
# to go on black aida etc
X_STITCH_FRAG = {
    RenderingAPI.OPENGL: """
    #version 330 core
    in vec2 textureCoord;
    out vec4 FragColor;

    uniform sampler2D cross_stitch_texture;

    void main() {
        FragColor = texture(cross_stitch_texture, textureCoord);
    }""",
    RenderingAPI.GLES: """""",
    RenderingAPI.METAL: """
    using namespace metal;

    struct VertexOut {
        float4 position [[position]];
        float2 tex;
    };

    fragment float4 fragment_main(VertexOut vert [[stage_in]],
                                  texture2d<float, access::sample> cross_stitch_texture,
                                  sampler cross_stitch_sampler) {
        return cross_stitch_texture.sample(cross_stitch_sampler, vert.tex);
    }"""
}


class CanvasRenderer:
    def __init__(self, app, width, height) -> None:
        self.app = app
        self.data = None
        self.texture_data = None
        self.camera = Camera2D(app, width, height)
        self.mouse_position = None

        # key is Thread.global_identifier
        self.data = np.full((width, height), fill_value="", dtype=np.object_)
        # since alpha is 0, no pixels are visible yet
        self.texture_data = np.zeros((height, width, 4), dtype=np.uint8)

        self.cross_stitch_texture = nanogui.Texture(
            pixel_format=nanogui.Texture.PixelFormat.RGBA,
            component_format=nanogui.Texture.ComponentFormat.UInt8,
            size=(width, height),
            mag_interpolation_mode=nanogui.Texture.InterpolationMode.Nearest,
            min_interpolation_mode=nanogui.Texture.InterpolationMode.Bilinear
        )
        self.cross_stitch_texture.upload(self.texture_data)

        self.render_pass = nanogui.RenderPass([app])
        self.render_pass.set_clear_color(0, nanogui.Color(0.3, 0.3, 0.32, 1.0))
        self.render_pass.set_cull_mode(nanogui.RenderPass.CullMode.Disabled)

        if nanogui.api == "opengl":
            rendering_api = RenderingAPI.OPENGL
        elif nanogui.api == "metal":
            rendering_api = RenderingAPI.METAL
        elif nanogui.api == "gles2" or nanogui.api == "gles3":
            raise NotImplementedError

        self.cs_shader = nanogui.Shader(
            self.render_pass, "cross stitches",
            X_STITCH_VERT[rendering_api], X_STITCH_FRAG[rendering_api])

        self.minor_grid_shader = nanogui.Shader(
            self.render_pass, "minor_grid",
            GRID_VERT[rendering_api], GRID_FRAG[rendering_api])

        self.major_grid_shader = nanogui.Shader(
            self.render_pass, "major_grid",
            GRID_VERT[rendering_api], GRID_FRAG[rendering_api])

        h = 1.0
        v = 1.0

        if width > height:
            v = (height / width)
        else:
            h = (width / height)

        self.canvas_height_ndc = v * 2

        self.position = np.array(
            [[-h, -v, 0],  # bottom left
             [ h, -v, 0],  # bottom right
             [ h,  v, 0],  # top right
             [-h,  v, 0]], # top left
            dtype=np.float32)
        tex = np.array(
            [0, 0,  # bottom left
             1, 0,  # bottom right
             1, 1,  # top right
             0, 1], # top left
            dtype=np.float32)
        indices = np.array(
            [0, 1, 2,
             2, 3, 0],
            dtype=np.uint32)

        self.cs_shader.set_buffer("position", self.position)
        self.cs_shader.set_buffer("tex", tex)
        self.cs_shader.set_buffer("indices", indices)
        self.cs_shader.set_texture("cross_stitch_texture", self.cross_stitch_texture)

        self.minor_grid_mark_distance = 2 / max(width, height)
        major_grid_mark_distance = self.minor_grid_mark_distance * 10
        major_grid_mark_normal = self.minor_grid_mark_distance * 0.125

        width_minus_1 = width - 1
        height_minus_1 = height - 1

        minor_horizontal_gridmarks = np.empty((height_minus_1 * 2, 2), dtype=np.float32)
        minor_vertical_gridmarks = np.empty((width_minus_1 * 2, 2), dtype=np.float32)

        major_horizontal_gridmarks = np.empty(((height_minus_1 // 10) * 4, 2), dtype=np.float32)
        major_vertical_gridmarks = np.empty(((width_minus_1 // 10) * 4, 2), dtype=np.float32)

        x = -h
        for i in range(width_minus_1):
            x += self.minor_grid_mark_distance
            minor_vertical_gridmarks[2*i] =       [x,  v]
            minor_vertical_gridmarks[(2*i) + 1] = [x, -v]

        x = -h
        for i in range(width_minus_1 // 10):
            x += major_grid_mark_distance
            major_vertical_gridmarks[4*i] =       [x - major_grid_mark_normal,  v]
            major_vertical_gridmarks[(4*i) + 1] = [x - major_grid_mark_normal, -v]
            major_vertical_gridmarks[(4*i) + 2] = [x + major_grid_mark_normal,  v]
            major_vertical_gridmarks[(4*i) + 3] = [x + major_grid_mark_normal, -v]

        y = -v
        for i in range(height_minus_1):
            y += self.minor_grid_mark_distance
            minor_horizontal_gridmarks[2*i] =       [ h, y]
            minor_horizontal_gridmarks[(2*i) + 1] = [-h, y]

        y = -v
        for i in range(height_minus_1 // 10):
            y += major_grid_mark_distance
            major_horizontal_gridmarks[4*i] =       [ h, y - major_grid_mark_normal]
            major_horizontal_gridmarks[(4*i) + 1] = [-h, y - major_grid_mark_normal]
            major_horizontal_gridmarks[(4*i) + 2] = [ h, y + major_grid_mark_normal]
            major_horizontal_gridmarks[(4*i) + 3] = [-h, y + major_grid_mark_normal]

        minor_gridmarks = np.concatenate((minor_vertical_gridmarks, minor_horizontal_gridmarks))
        major_gridmarks = np.concatenate((major_horizontal_gridmarks, major_vertical_gridmarks))
        self.minor_grid_total_verts, _ = minor_gridmarks.shape
        major_grid_total_verts, _ = major_gridmarks.shape


        major_grid_indices = np.array([[x + 0, x + 1, x + 2,
                                        x + 2, x + 1, x + 3] for x in range(0, major_grid_total_verts, 4)],
                                       dtype=np.uint32).flatten()

        self.major_grid_indices_size = major_grid_indices.shape[0]

        self.minor_grid_shader.set_buffer("position", minor_gridmarks)
        self.minor_grid_shader.set_buffer("colour", np.array([0.5, 0.5, 0.5, 1.0], dtype=np.float32))

        if self.major_grid_indices_size > 0:
            self.major_grid_shader.set_buffer("position", major_gridmarks)
            self.major_grid_shader.set_buffer("indices", major_grid_indices)
            self.major_grid_shader.set_buffer("colour", np.array([0.0, 0.0, 0.0, 1.0], dtype=np.float32))
        else:
            self.major_grid_shader = None

    def find_mouse_position(self, left, right, top, bottom):
        mouse_ndc = self.camera.screen_to_ndc(*self.app.mouse_pos())
        if mouse_ndc is None:
            self.mouse_position = None
            return

        mouse_normalised_x, mouse_normalised_y = mouse_ndc
        self.mouse_position = self.camera.ndc_to_stitch(mouse_normalised_x, mouse_normalised_y, left, right, top, bottom)

    def draw_to_canvas(self, x, y, colour):
        self.data[x][y] = colour.global_identifier
        self.texture_data[y][x] = colour.colour

        self.cross_stitch_texture.upload(self.texture_data)

    def erase_from_canvas(self, x, y):
        self.data[x][y] = ""
        self.texture_data[y][x] = [0, 0, 0, 0]

        self.cross_stitch_texture.upload(self.texture_data)

    def render(self):
        device_size = self.app.framebuffer_size()
        self.render_pass.resize(device_size)

        left, right, top, bottom = self.camera.canvas_bounds(self.position)
        self.find_mouse_position(left, right, top, bottom)

        # Don't need to account for orthographic projection on vertical axis, since it isn't adjusted
        pixel_canvas_height = self.canvas_height_ndc * self.camera.scale * device_size[1] / self.app.pixel_ratio()
        minor_grid_mark_pixel_distance = pixel_canvas_height * self.minor_grid_mark_distance

        with self.render_pass:
            self.render_cs_shader()
            if minor_grid_mark_pixel_distance >= 15:
                self.render_minor_grid_shader()
                self.render_major_grid_shader()

    def render_cs_shader(self):
        if self.cs_shader is None:
            return

        mvp = self.camera.generate_matrices()
        self.cs_shader.set_buffer("mvp", mvp.T)

        with self.cs_shader:
            self.cs_shader.draw_array(nanogui.Shader.PrimitiveType.Triangle, 0, 6, True)

    def render_minor_grid_shader(self):
        if self.minor_grid_shader is None:
            return

        mvp = self.camera.generate_matrices()
        self.minor_grid_shader.set_buffer("mvp", mvp.T)

        with self.minor_grid_shader:
            self.minor_grid_shader.draw_array(nanogui.Shader.PrimitiveType.Line, 0,
                                              self.minor_grid_total_verts, False)

    def render_major_grid_shader(self):
        if self.major_grid_shader is None:
            return

        mvp = self.camera.generate_matrices()
        self.major_grid_shader.set_buffer("mvp", mvp.T)

        with self.major_grid_shader:
            self.major_grid_shader.draw_array(nanogui.Shader.PrimitiveType.Triangle, 0,
                                              self.major_grid_indices_size, True)
