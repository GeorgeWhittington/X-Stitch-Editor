import time
import sys

import nanogui

from app import GLOBAL_THREAD_LOOKUP, CanvasRenderer
from app.windows import ToolWindow, CreateNewProjectWindow, SplashscreenWindow, MousePositionWindow

class LargeIconTheme(nanogui.Theme):
    def __init__(self, ctx):
        super(LargeIconTheme, self).__init__(ctx)
        self.m_icon_scale = 0.9


class XStitchEditorApp(nanogui.Screen):
    ORIGINAL_WIDTH = 1024
    ORIGINAL_HEIGHT = 748

    def __init__(self):
        super(XStitchEditorApp, self).__init__((self.ORIGINAL_WIDTH, self.ORIGINAL_HEIGHT), "X Stitch Editor")

        # gui options
        self.selected_thread = None
        self.selected_tool = None

        self.shader = None
        self.project = None
        self.canvas_renderer = None
        self.last_frame = 0.0
        self.delta_time = 0.0

        large_icon_theme = LargeIconTheme(self.nvg_context())
        self.set_theme(large_icon_theme)

        self.tool_window = ToolWindow(self)
        self.mouse_pos_window = MousePositionWindow(self)
        self.splashscreen_window = SplashscreenWindow(self)

        self.new_project_window = CreateNewProjectWindow(self)

        self.perform_layout()

    def switch_project(self, project):
        # TODO: check if a project already exists, modal about saving it if so

        # TODO: use bg colour + palette data
        self.project = project
        self.canvas_renderer = CanvasRenderer(self, project.width, project.height)

    def draw_contents(self):
        # Keep movement consistent with framerate
        current_frame = time.time()
        self.delta_time = current_frame - self.last_frame
        self.last_frame = current_frame

        if self.canvas_renderer is None:
            super(XStitchEditorApp, self).draw_contents()
            return

        self.canvas_renderer.render()

        if self.canvas_renderer.mouse_position:
            stitch_x, stitch_y = self.canvas_renderer.mouse_position
            colour = GLOBAL_THREAD_LOOKUP.get(self.canvas_renderer.data[stitch_x, stitch_y], None)
            _, frame_height = self.framebuffer_size()
            self.mouse_pos_window.set_captions(stitch_x + 1, stitch_y + 1, frame_height / self.pixel_ratio(), colour)
        else:
            self.mouse_pos_window.set_visible(False)
        self.perform_layout()

    # ~~~~ Window Events ~~~~

    def keyboard_event(self, key, scancode, action, modifiers):
        if super(XStitchEditorApp, self).keyboard_event(key, scancode,
                                                        action, modifiers):
            return True

        if key == nanogui.glfw.KEY_ESCAPE and action == nanogui.glfw.PRESS:
            self.set_visible(False)
            return True

        if self.canvas_renderer is None:
            return False

        camera = self.canvas_renderer.camera
        camera_speed = 2 * self.delta_time

        if key == nanogui.glfw.KEY_LEFT:
            camera.pan_camera(camera_speed, 0, self.canvas_renderer.position)
        if key == nanogui.glfw.KEY_RIGHT:
            camera.pan_camera(-camera_speed, 0, self.canvas_renderer.position)
        if key == nanogui.glfw.KEY_UP:
            camera.pan_camera(0, -camera_speed, self.canvas_renderer.position)
        if key == nanogui.glfw.KEY_DOWN:
            camera.pan_camera(0, camera_speed, self.canvas_renderer.position)

        control_command_key = nanogui.glfw.MOD_SUPER if sys.platform.startswith("darwin") else nanogui.glfw.MOD_CONTROL

        if key == nanogui.glfw.KEY_EQUAL and modifiers == control_command_key:
            camera.zoom_to_point(0, 0, 1 + camera_speed, self.canvas_renderer.position)
        if key == nanogui.glfw.KEY_MINUS and modifiers == control_command_key:
            camera.zoom_to_point(0, 0, 1 - camera_speed, self.canvas_renderer.position)

        return False

    def scroll_event(self, mouse_position, delta):
        if super(XStitchEditorApp, self).scroll_event(mouse_position, delta):
            return True

        if self.canvas_renderer is None:
            return False

        # We only care about vertical scroll
        if delta[1]:
            camera = self.canvas_renderer.camera
            mouse_ndc = camera.screen_to_ortho_ndc(*mouse_position)

            zoom_factor = 1 + (delta[1] * self.delta_time)
            camera.zoom_to_point(*mouse_ndc, zoom_factor, self.canvas_renderer.position)

        return False

    # TODO: start tracking mouse location and only enable the cursor for a specific tool where it
    # makes sense. eg:
    # single stitch tool: when outside canvas bounds, enable hand cursor
    # (ditto for back stitch and fill)

    def mouse_button_event(self, position, button, down, modifiers):
        if super(XStitchEditorApp, self).mouse_button_event(position, button, down, modifiers):
            return True

        if self.canvas_renderer is None:
            return False

        if button == nanogui.glfw.MOUSE_BUTTON_1 and down:
            camera = self.canvas_renderer.camera
            mouse_ndc = camera.screen_to_ndc(*position)
            if mouse_ndc is None:
                return False

            if self.selected_tool == "ZOOM_IN":
                zoom_factor = 1 + (0.1)
                camera.zoom_to_point(*mouse_ndc, zoom_factor, self.canvas_renderer.position)
                return False
            if self.selected_tool == "ZOOM_OUT":
                zoom_factor = 1 - (0.1)
                camera.zoom_to_point(*mouse_ndc, zoom_factor, self.canvas_renderer.position)
                return False

            if self.tool_window.mouse_over(*position):
                return False

            left, right, top, bottom = camera.canvas_bounds(self.canvas_renderer.position)
            selected_stitch = camera.ndc_to_stitch(*mouse_ndc, left, right, top, bottom)

            if selected_stitch is None:
                return False

            if self.selected_tool == "SINGLE_STITCH":
                if self.selected_thread:
                    self.canvas_renderer.draw_to_canvas(*selected_stitch, self.selected_thread)
            elif self.selected_tool == "BACK_STITCH":
                pass  # TODO
            elif self.selected_tool == "ERASE":
                self.canvas_renderer.erase_from_canvas(*selected_stitch)
            elif self.selected_tool == "FILL":
                pass  # TODO
        return False

    def mouse_motion_event(self, position, rel, button, modifiers):
        if super(XStitchEditorApp, self).mouse_motion_event(position, rel, button, modifiers):
            return True

        if self.canvas_renderer is None:
            return False

        # TODO: check that this gets the right result on windows/linux and
        # that it isn't just a weird glfw quirk
        button_1 = nanogui.glfw.MOUSE_BUTTON_2 if sys.platform.startswith("darwin") else nanogui.glfw.MOUSE_BUTTON_1
        camera = self.canvas_renderer.camera

        if button == button_1:
            mouse_ndc = camera.screen_to_ndc(*position)
            if mouse_ndc is None:
                return False

            if self.selected_tool == "MOVE":
                prev_position = (position[0] - rel[0], position[1] - rel[1])
                prev_ndc = camera.screen_to_ortho_ndc(*prev_position)
                current_ndc = camera.screen_to_ortho_ndc(*position)

                if prev_ndc is None:
                    return False

                camera.pan_camera(current_ndc[0] - prev_ndc[0], current_ndc[1] - prev_ndc[1], self.canvas_renderer.position)

            if self.tool_window.mouse_over(*position):
                return False

            left, right, top, bottom = camera.canvas_bounds(self.canvas_renderer.position)
            selected_stitch = camera.ndc_to_stitch(*mouse_ndc, left, right, top, bottom)

            if selected_stitch is None:
                # TODO: if a drawing tool rather than a zoom tool is selected, it would
                # make sense to also move the camera here aswell. Change the cursor
                # during the duration of the drag to make it obvious

                return False

            if self.selected_tool == "SINGLE_STITCH":
                if self.selected_thread:
                    self.canvas_renderer.draw_to_canvas(*selected_stitch, self.selected_thread)
            if self.selected_tool == "ERASE":
                self.canvas_renderer.erase_from_canvas(*selected_stitch)

        return False

    def resize_event(self, size):
        self.spashscreen_window.center()
        self.new_project_window.center()
        return True