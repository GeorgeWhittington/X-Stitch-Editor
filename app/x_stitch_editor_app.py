import time
import sys

import nanogui

from app import MANUFACTURERS, Thread, CanvasRenderer, GLOBAL_THREAD_LOOKUP, Project


class LargeIconTheme(nanogui.Theme):
    def __init__(self, ctx):
        super(LargeIconTheme, self).__init__(ctx)
        self.m_icon_scale = 0.9


class DisabledButton(nanogui.Button):
    def mouse_button_event(self, *_):
        return False

    def mouse_enter_event(self, *_):
        return False


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

        self.create_tool_window()
        self.create_mouse_pos_window()
        self.create_splashscreen_window()

        self.create_new_project_window()

        self.perform_layout()

    # ~~~~ Window Creators ~~~~

    def create_tool_window(self):
        self.tool_window = nanogui.Window(self, "Tools")
        self.tool_window.set_position((0, 0))
        self.tool_window.set_layout(nanogui.GroupLayout(
            margin=5, spacing=5, group_spacing=10, group_indent=0))

        # Drawing Tools
        nanogui.Label(self.tool_window, "Tools", "sans-bold")
        tool_wrapper = nanogui.Widget(self.tool_window)
        tool_wrapper.set_layout(nanogui.BoxLayout(
            orientation=nanogui.Orientation.Horizontal,
            alignment=nanogui.Alignment.Fill, margin=0, spacing=0))

        tools = nanogui.Widget(tool_wrapper)
        tool_layout = nanogui.GridLayout(
            orientation=nanogui.Orientation.Horizontal, resolution=5,
            alignment=nanogui.Alignment.Fill, margin=0, spacing=0)
        tool_layout.set_spacing(0, 5)
        tools.set_layout(tool_layout)

        def make_tool_button_callback(tool, cursor=nanogui.Cursor.Arrow):
            def callback():
                self.selected_tool = tool
                self.set_cursor(cursor)
            return callback

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_HAND_PAPER)
        toolbutton.set_tooltip("Move")
        toolbutton.set_callback(make_tool_button_callback("MOVE", nanogui.Cursor.Hand))
        toolbutton.set_pushed(True)
        self.selected_tool = "MOVE"
        self.set_cursor(nanogui.Cursor.Hand)

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_PENCIL_ALT)
        toolbutton.set_tooltip("Single Stitch")
        toolbutton.set_callback(make_tool_button_callback("SINGLE_STITCH"))

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_PEN_NIB)
        toolbutton.set_tooltip("Back Stitch")
        toolbutton.set_callback(make_tool_button_callback("BACK_STITCH", nanogui.Cursor.Crosshair))

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_ERASER)
        toolbutton.set_tooltip("Erase")
        toolbutton.set_callback(make_tool_button_callback("ERASE"))

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_FILL_DRIP)
        toolbutton.set_tooltip("Fill Area")
        toolbutton.set_callback(make_tool_button_callback("FILL"))

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_SEARCH_PLUS)
        toolbutton.set_tooltip("Zoom In")
        toolbutton.set_callback(make_tool_button_callback("ZOOM_IN", nanogui.Cursor.VResize))

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_SEARCH_MINUS)
        toolbutton.set_tooltip("Zoom Out")
        toolbutton.set_callback(make_tool_button_callback("ZOOM_OUT", nanogui.Cursor.VResize))

        # Palette
        self.selected_thread_title = nanogui.Label(self.tool_window, "Selected Thread", "sans-bold")

        self.selected_thread_widget = nanogui.Widget(self.tool_window)
        self.selected_thread_widget.set_layout(nanogui.BoxLayout(
            orientation=nanogui.Orientation.Horizontal,
            alignment=nanogui.Alignment.Maximum, margin=0, spacing=5))

        self.selected_thread_button = DisabledButton(self.selected_thread_widget, "")
        self.selected_thread_label = nanogui.Label(self.selected_thread_widget, "")
        self.update_selected_thread_widget()

        nanogui.Label(self.tool_window, "Palette", "sans-bold")

        v_scroll = nanogui.VScrollPanel(self.tool_window)
        v_scroll.set_fixed_size((0, 470))

        palette_widget = nanogui.Widget(v_scroll)
        palette_layout = nanogui.GridLayout(
            orientation=nanogui.Orientation.Horizontal, resolution=3,
            alignment=nanogui.Alignment.Maximum, margin=0, spacing=5)
        palette_layout.set_col_alignment([nanogui.Alignment.Minimum, nanogui.Alignment.Minimum, nanogui.Alignment.Maximum])
        palette_widget.set_layout(palette_layout)

        # TODO: Read from project object
        selected_palette = None
        for p in MANUFACTURERS.values():
            selected_palette = p
            break

        def make_palette_button_callback(thread):
            def callback():
                self.selected_thread = thread
                self.update_selected_thread_widget()
            return callback

        nanogui.Label(palette_widget, "Company", "sans")
        nanogui.Label(palette_widget, "Code", "sans")
        nanogui.Label(palette_widget, "")

        for thread in selected_palette.values():
            nanogui.Label(palette_widget, thread.company, "sans")
            nanogui.Label(palette_widget, thread.number, "sans")

            pb = nanogui.Button(palette_widget, "  ")
            pb.set_background_color(nanogui.Color(*thread.colour))
            pb.set_callback(make_palette_button_callback(thread))
            pb.set_tooltip(thread.description)

        # TODO: switch/edit palette button that launches a popup to do so?

        self.tool_window.set_visible(False)

    def create_mouse_pos_window(self):
        self.mouse_pos_window = nanogui.Window(self, "")
        self.mouse_pos_window.set_layout(nanogui.GroupLayout(
            margin=5, spacing=5, group_spacing=0, group_indent=0))
        self.mouse_location_label = nanogui.Label(self.mouse_pos_window, "")
        self.mouse_location_label_2 = nanogui.Label(self.mouse_pos_window, "")
        self.mouse_pos_window.set_visible(False)

    def create_splashscreen_window(self):
        self.spashscreen_window = nanogui.Window(self, "")
        self.spashscreen_window.set_layout(nanogui.GroupLayout(
            margin=5, spacing=5, group_spacing=10, group_indent=0))

        def create_project():
            self.spashscreen_window.set_visible(False)
            self.new_project_window.set_visible(True)
            self.perform_layout()

        def convert_image():
            print("convert image")

        def load_project():
            print("load project")

        create_project_button = nanogui.Button(self.spashscreen_window, "Create new project")
        create_project_button.set_callback(create_project)
        create_project_button.set_font_size(30)

        convert_image_button = nanogui.Button(self.spashscreen_window, "Create new project from an image")
        convert_image_button.set_callback(convert_image)
        convert_image_button.set_font_size(30)

        load_project_button = nanogui.Button(self.spashscreen_window, "Load a project")
        load_project_button.set_callback(load_project)
        load_project_button.set_font_size(30)

        self.spashscreen_window.center()

    def create_new_project_window(self):
        self.new_project_window = nanogui.Window(self, "")
        self.new_project_window.set_layout(nanogui.BoxLayout(
            nanogui.Orientation.Vertical, nanogui.Alignment.Middle,
            margin=15, spacing=5))

        errors = nanogui.Label(self.new_project_window, "")
        errors.set_color(nanogui.Color(204, 0, 0, 255))
        errors.set_visible(False)

        form_widget = nanogui.Widget(self.new_project_window)
        layout = nanogui.GridLayout(nanogui.Orientation.Horizontal, 2,
                                    nanogui.Alignment.Middle, margin=0, spacing=5)
        layout.set_col_alignment([nanogui.Alignment.Fill, nanogui.Alignment.Fill])
        form_widget.set_layout(layout)

        nanogui.Label(form_widget, "Project Title:")
        title = nanogui.TextBox(form_widget, "")
        title.set_editable(True)
        title.set_alignment(nanogui.TextBox.Alignment.Left)

        nanogui.Label(form_widget, "Canvas Width:")
        width = nanogui.IntBox(form_widget, 100)

        nanogui.Label(form_widget, "Canvas Height:")
        height = nanogui.IntBox(form_widget, 100)

        for intbox in [width, height]:
            intbox.set_default_value("100")
            intbox.set_units("stitches")
            intbox.set_editable(True)
            intbox.set_alignment(nanogui.TextBox.Alignment.Left)
            intbox.set_fixed_size((200, 0))
            intbox.set_value_increment(10)
            intbox.set_spinnable(True)
            intbox.set_min_value(1)

        nanogui.Label(form_widget, "Canvas Background Colour:")
        color = nanogui.ColorPicker(form_widget, nanogui.Color(255, 255, 255, 255))

        # TODO: select palette from list?

        def create_new_project():
            errors.set_visible(False)

            try:
                project = Project(
                    title.value(), width.value(), height.value(),
                    color.color, None)
            except ValueError as err:
                errors.set_caption(str(err))
                errors.set_visible(True)
                self.perform_layout()
                return

            # TODO: use bg colour + palette data
            self.project = project
            self.canvas_renderer = CanvasRenderer(self, project.width, project.height)

            # reset form values
            title.set_value("")
            width.set_value(100)
            height.set_value(100)
            color.set_color(nanogui.Color(255, 255, 255, 255))

            self.new_project_window.set_visible(False)
            self.tool_window.set_visible(True)
            self.perform_layout()

        confirm_button = nanogui.Button(self.new_project_window, "Confirm")
        confirm_button.set_callback(create_new_project)

        self.new_project_window.center()
        self.new_project_window.set_visible(False)

    # ~~~~ Misc ~~~~

    def update_selected_thread_widget(self):
        if self.selected_thread is None:
            self.selected_thread_button.set_icon(nanogui.icons.FA_BAN)
            self.selected_thread_button.set_caption("")
        else:
            self.selected_thread_button.set_icon(0)
            self.selected_thread_label.set_caption(self.selected_thread.number)
            self.selected_thread_button.set_background_color(nanogui.Color(*self.selected_thread.colour))
            self.selected_thread_button.set_caption("  ")

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
            if colour:
                self.mouse_location_label.set_caption(f"stitch selected: {stitch_x + 1}, {stitch_y + 1}")
                self.mouse_location_label_2.set_caption(f"thread: {colour.company} {colour.number}")
                self.mouse_pos_window.set_position((0, int((frame_height / self.pixel_ratio()) - 42)))
            else:
                self.mouse_location_label.set_caption(f"stitch selected: {stitch_x + 1}, {stitch_y + 1}")
                self.mouse_location_label_2.set_caption("")
                self.mouse_pos_window.set_position((0, int((frame_height / self.pixel_ratio()) - 26)))
            self.mouse_pos_window.set_visible(True)
        else:
            self.mouse_pos_window.set_visible(False)
        self.perform_layout()

    def over_tool_window(self, x, y):
        control_x, control_y = self.tool_window.absolute_position()
        control_width, control_height = self.tool_window.size()

        return x >= control_x and y >= control_y and \
               x <= control_x + control_width and y <= control_y + control_height

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

            if self.over_tool_window(*position):
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

            if self.over_tool_window(*position):
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