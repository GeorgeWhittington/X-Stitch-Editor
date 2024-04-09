from typing import Optional, Tuple

import nanogui
import glm


class Camera2D:
    def __init__(self, app, width, height) -> None:
        self.app = app
        self.width = width
        self.height = height

        self.translation_x = 0.0
        self.translation_y = 0.0
        self.scale = 1.0

        self.generate_matrices(first_render=True)

    def pan_camera(self, dx, dy, position):
        new_x = self.translation_x + (dx / self.scale)
        new_y = self.translation_y + (dy / self.scale)

        left, right, top, bottom = self.translated_canvas_bounds(position, new_x, new_y, self.scale)

        if left > 0.8 or right < -0.8 or top < -0.8 or bottom > 0.8:
            return False

        self.translation_x = new_x
        self.translation_y = new_y
        return True

    def zoom_camera(self, zoom, position):
        new_scale = self.scale * zoom

        left, right, top, bottom = self.translated_canvas_bounds(position, self.translation_x, self.translation_y, zoom)

        if left > 0.8 or right < -0.8 or top < -0.8 or bottom > 0.8:
            return False

        self.scale = max(self.minimum_scale, min(self.maximum_scale, new_scale))
        return True

    def zoom_to_point(self, x, y, zoom, position):
        success = self.pan_camera(-x, -y, position)
        if not success:
            return

        prev_scale = self.scale
        success = self.zoom_camera(zoom, position)
        if not success:
            # undo all previous steps
            self.pan_camera(x, y, position)
            return

        success = self.pan_camera(x, y, position)
        if not success:
            # undo all previous steps
            self.zoom_camera(prev_scale / self.scale, position)
            self.pan_camera(x, y, position)

    def calculate_base_scale(self, device_height):
        # TODO: is this actually giving the scale needed to make canvas stitches 1 to 1 with screen pixels?
        return self.height / device_height * self.app.pixel_ratio()

    def generate_matrices(self, first_render = False) -> nanogui.Matrix4f:
        current_device_size = self.app.framebuffer_size()
        device_ratio = current_device_size[0] / current_device_size[1]
        if first_render:
            self.device_size = current_device_size
            self.scale = self.calculate_base_scale(current_device_size[1])
            self.minimum_scale = self.scale / 2
            self.maximum_scale = self.scale * 200
            self.scale *= 2
        elif self.device_size != current_device_size:
            # clamp zoom on window re-size
            self.device_size = current_device_size
            base_scale = self.calculate_base_scale(current_device_size[1])
            self.minimum_scale = base_scale / 2
            self.maximum_scale = base_scale * 200
            self.scale = max(self.minimum_scale, min(self.maximum_scale, self.scale))

        trans = nanogui.Matrix4f.translate([self.translation_x, self.translation_y, 0.0])
        scale = nanogui.Matrix4f.scale([self.scale, self.scale, 1.0])
        ortho = nanogui.Matrix4f.ortho(
            -1 * device_ratio, 1 * device_ratio, # left, right
            -1, 1, -100, 100)                    # bottom, top, near, far

        return ortho @ scale @ trans

    def canvas_bounds(self, position) -> (float, float, float, float):
        """Returns the outermost points of the canvas in ndc coordinate
        space after being transformed by the mvc matrix."""
        current_device_size = self.app.framebuffer_size()
        device_ratio = current_device_size[0] / current_device_size[1]

        trans = glm.translate([self.translation_x, self.translation_y, 0.0])
        scale = glm.scale([self.scale, self.scale, 1.0])
        ortho = glm.ortho(
            -1 * device_ratio, 1 * device_ratio,
            -1, 1, -100, 100)

        mvc = ortho * scale * trans
        right, bottom, _, _ = mvc * glm.vec4(*position[1], 1.0)
        left, top, _, _ = mvc * glm.vec4(*position[3], 1.0)
        return left, right, top, bottom

    def translated_canvas_bounds(self, position, trans_x, trans_y, zoom) -> (float, float, float, float):
        current_device_size = self.app.framebuffer_size()
        device_ratio = current_device_size[0] / current_device_size[1]

        trans = glm.translate([trans_x, trans_y, 0.0])
        scale = glm.scale([zoom, zoom, 1.0])
        ortho = glm.ortho(
            -1 * device_ratio, 1 * device_ratio,
            -1, 1, -100, 100)

        mvc = ortho * scale * trans
        right, bottom, _, _ = mvc * glm.vec4(*position[1], 1.0)
        left, top, _, _ = mvc * glm.vec4(*position[3], 1.0)

        return left, right, top, bottom

    def get_scaled_device_dimensions(self) -> (int, int):
        device_size = self.app.framebuffer_size()
        pixel_ratio = self.app.pixel_ratio()
        device_width = device_size[0] / pixel_ratio
        device_height = device_size[1] / pixel_ratio
        return int(device_width), int(device_height)

    def device_coords_in_bounds(self, x, y) -> bool:
        device_width, device_height = self.get_scaled_device_dimensions()
        return x >= 0 and y >= 0 and x <= device_width and y <= device_height

    def screen_to_ndc(self, x, y) -> Optional[Tuple[float, float]]:
        """Returns the normalized device coordinates of a specified point on
        the screen, returning None if the point is outside the bounds of the window"""
        device_width, device_height = self.get_scaled_device_dimensions()

        if not self.device_coords_in_bounds(x, y):
            return None

        # normalise to -1, 1 https://stats.stackexchange.com/questions/178626/how-to-normalize-data-between-1-and-1
        normalised_x = (2 * (x / device_width)) - 1
        normalised_y = (2 * ((device_height - y) / device_height)) - 1  # y needs to be inverted

        return normalised_x, normalised_y

    def screen_to_ortho_ndc(self, x, y) -> Optional[Tuple[float, float]]:
        """Returns the normalized device coordinates of a specified point on
        the screen, which is then scaled as it would be by the orthographic
        projection, returning None if the point is outside the bounds of the window"""
        if not self.device_coords_in_bounds(x, y):
            return None

        device_width, device_height = self.get_scaled_device_dimensions()

        device_ratio = device_width / device_height

        # normalise x to -device_ratio, device_ratio and y to -1, 1
        normalised_x = (2 * device_ratio * (x / device_width)) - device_ratio
        normalised_y = (2 * ((device_height - y) / device_height)) - 1  # y needs to be inverted

        return normalised_x, normalised_y

    def inside_canvas(self, x, y, left, right, top, bottom):
        return x >= left and x <= right and y >= bottom and y <= top

    def ndc_to_stitch(self, x, y, left, right, top, bottom) -> Optional[Tuple[int, int]]:
        if not self.inside_canvas(x, y, left, right, top, bottom):
            return None

        stitch_x = int(self.width * ((x - left) / (right - left)))
        stitch_y = int(self.height * ((y - bottom) / (top - bottom)))

        stitch_x = max(0, min(self.width - 1, stitch_x))
        stitch_y = max(0, min(self.height - 1, stitch_y))
        return stitch_x, stitch_y
