from enum import Enum

import nanogui
import numpy as np

from app import GLOBAL_THREAD_LOOKUP

RenderingAPI = Enum("RenderingAPI", ["OPENGL", "GLES", "METAL"])

X_STITCH_VERT = {
    RenderingAPI.OPENGL: """
    #version 330 core
    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 tex;

    out vec2 textureCoord;

    uniform mat4 mvp;

    void main () {
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

# TODO: colour transparent pixels white
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
    def __init__(self, app, width = None, height = None) -> None:
        self.app = app
        self.width = width
        self.height = height
        self.zoom = 0.0
        self.translation_x = 0.0
        self.translation_y = 0.0
        self.data = None
        self.texture_data = None

        if width and height:
            # key is Thread.global_identifier
            self.data = np.full((width, height), fill_value="", dtype=np.object_)
            # since alpha is 0, no pixels are visible yet
            self.texture_data = np.zeros((width, height, 4), dtype=np.uint8)

            self.texture_data[0, 0] = [255, 0, 0, 255]

            self.cross_stitch_texture = nanogui.Texture(
                pixel_format=nanogui.Texture.PixelFormat.RGBA,
                component_format=nanogui.Texture.ComponentFormat.UInt8,
                size=self.texture_data.shape[:2],
                mag_interpolation_mode=nanogui.Texture.InterpolationMode.Nearest,
                min_interpolation_mode=nanogui.Texture.InterpolationMode.Bilinear
            )
            self.cross_stitch_texture.upload(self.texture_data)

        self.render_pass = nanogui.RenderPass([app])
        self.render_pass.set_clear_color(0, nanogui.Color(0.3, 0.3, 0.32, 1.0))

        if nanogui.api == "opengl":
            rendering_api = RenderingAPI.OPENGL
        elif nanogui.api == "metal":
            rendering_api = RenderingAPI.METAL
        elif nanogui.api == "gles2" or nanogui.api == "gles3":
            raise NotImplementedError

        self.cs_shader = nanogui.Shader(
            self.render_pass, "cross stitches",
            X_STITCH_VERT[rendering_api], X_STITCH_FRAG[rendering_api])

        indices = np.array(
            [0, 1, 2,
             2, 3, 0],
            dtype=np.uint32)
        self.cs_shader.set_buffer("indices", indices)

        tex = np.array([
            0, 0,  # bottom left
            1, 0,  # bottom right
            1, 1,  # top right
            0, 1   # top left
        ], dtype=np.float32)
        self.cs_shader.set_buffer("tex", tex)

        self.cs_shader.set_texture("cross_stitch_texture", self.cross_stitch_texture)

        h = 1.0
        v = 1.0

        if width and height:
            if width > height:
                v = (height / width) * v
            else:
                h = (width / height) * h

        position = np.array(
            [[-h, -v, 0],  # bottom left
             [ h, -v, 0],  # bottom right
             [ h,  v, 0],  # top right
             [-h,  v, 0]], # top left
            dtype=np.float32)
        self.cs_shader.set_buffer("position", position)

    def render(self):
        device_size = self.app.framebuffer_size()
        self.render_pass.resize(device_size)

        with self.render_pass:
            if not self.width or not self.height:
                return  # if there is no canvas size, do not render

            self.render_cs_shader(device_size[0], device_size[1])

    def render_cs_shader(self, device_width, device_height):
        if self.cs_shader is None:
            return

        # TODO: matrix is calculated on each frame, should there
        #instead be a check for if something has actually changed?
        device_ratio = device_width / device_height
        base_scale = (self.height / device_height) * self.app.pixel_ratio()
        scale = base_scale + self.zoom

        # TODO: sanity check zoom here
        # I can probably calculate a sensible max and minimum and then bound
        # it with that, while also accounting for overflow

        trans = nanogui.Matrix4f.translate([self.translation_x, self.translation_y, 0.0])
        scale = nanogui.Matrix4f.scale([scale, scale, 1.0])
        ortho = nanogui.Matrix4f.ortho(
            -1 * device_ratio, 1 * device_ratio, # left, right
            -1, 1, -100, 100)                    # bottom, top, near, far

        mvp = ortho @ trans @ scale
        self.cs_shader.set_buffer("mvp", mvp.T)

        with self.cs_shader:
            self.cs_shader.draw_array(nanogui.Shader.PrimitiveType.Triangle, 0, 6, True)
