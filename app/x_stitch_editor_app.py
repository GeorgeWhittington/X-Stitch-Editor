import time

import nanogui

from app import MANUFACTURERS, Thread, CanvasRenderer


class LargeIconTheme(nanogui.Theme):
    def __init__(self, ctx):
        super(LargeIconTheme, self).__init__(ctx)
        self.m_icon_scale = 0.9


class PaletteButton(nanogui.Button):
    def __init__(self, ctx, thread: Thread, app: "XStitchEditorApp"):
        super(PaletteButton, self).__init__(ctx, caption="  ")
        self.thread_colour = thread
        self.app = app
        self.set_background_color(nanogui.Color(*thread.colour))
        self.set_callback(self.thread_colour_callback)

    def thread_colour_callback(self):
        # For some godforsaken reason, callbacks are called on instantiation,
        # this just makes sure buttons aren't pressed before the ui has first rendered
        if not self.app.initialised:
            return
        self.app.selected_thread = self.thread_colour
        self.app.update_selected_thread_widget()


class XStitchEditorApp(nanogui.Screen):
    ORIGINAL_WIDTH = 1024
    ORIGINAL_HEIGHT = 748

    def __init__(self):
        super(XStitchEditorApp, self).__init__((self.ORIGINAL_WIDTH, self.ORIGINAL_HEIGHT), "X Stitch Editor")

        # gui options
        self.selected_thread = None
        self.selected_tool = None

        self.initialised = False
        self.shader = None
        self.canvas_renderer = CanvasRenderer(self, 20, 20)
        self.last_frame = 0.0
        self.delta_time = 0.0

        large_icon_theme = LargeIconTheme(self.nvg_context())
        self.set_theme(large_icon_theme)

        window = nanogui.Window(self, "Tools")
        window.set_position((0, 0))
        window.set_layout(nanogui.GroupLayout(
            margin=5, spacing=5, group_spacing=10, group_indent=0))

        # Drawing Tools
        nanogui.Label(window, "Drawing Tools", "sans-bold")
        tools = nanogui.Widget(window)
        tools.set_layout(nanogui.BoxLayout(
            nanogui.Orientation.Horizontal,
            nanogui.Alignment.Middle, margin=0, spacing=6))

        def single_stitch_callback():
            self.selected_tool = "SINGLE_STITCH"

        def back_stitch_callback():
            self.selected_tool = "BACK_STITCH"

        def brush_callback():
            self.selected_tool = "BRUSH"

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_PENCIL_ALT)
        toolbutton.set_tooltip("Single Stitch")
        toolbutton.set_callback(single_stitch_callback)

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_PEN_NIB)
        toolbutton.set_tooltip("Back Stitch")
        toolbutton.set_callback(back_stitch_callback)

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_PAINT_BRUSH)
        toolbutton.set_tooltip("Brush")
        toolbutton.set_callback(brush_callback)

        # TODO: Eraser

        # Palette
        nanogui.Label(window, "Palette", "sans-bold")

        self.selected_thread_widget = nanogui.Widget(window)
        self.selected_thread_widget.set_layout(nanogui.BoxLayout(
            nanogui.Orientation.Horizontal, nanogui.Alignment.Maximum,
            margin=5, spacing=5))
        self.selected_thread_label = nanogui.Label(self.selected_thread_widget, "Selected Thread: ")
        self.selected_thread_button = nanogui.Button(self.selected_thread_widget, "  ")
        # TODO: figure out how to disable this button (mouse_button_event?)

        self.selected_thread_widget.set_visible(False)

        v_scroll = nanogui.VScrollPanel(window)
        v_scroll.set_fixed_size((0, 490))

        palette_widget = nanogui.Widget(v_scroll)
        palette_widget.set_layout(nanogui.GridLayout(
            orientation=nanogui.Orientation.Horizontal, resolution=2,
            alignment=nanogui.Alignment.Maximum, margin=5, spacing=5))

        # TODO: Load this from a setting
        selected_palette = None
        for p in MANUFACTURERS.values():
            selected_palette = p
            break

        for thread in selected_palette.values():
            nanogui.Label(palette_widget, f"{thread.description} - {thread.number}", "sans")
            PaletteButton(palette_widget, thread, self)

        # ----------------- UI LAYOUT COMPLETE ------------------
        self.perform_layout()
        self.initialised = True
        # TODO: fix layout jumping when selected thread is changed by saving the dimensions
        # calculated here and enforcing them

    def update_selected_thread_widget(self):
        if self.selected_thread is None:
            self.selected_thread_widget.set_visible(False)
            self.perform_layout()
            return

        self.selected_thread_label.set_caption(f"Selected Thread: {self.selected_thread.description} - {self.selected_thread.number}")
        self.selected_thread_button.set_background_color(nanogui.Color(*self.selected_thread.colour))

        self.selected_thread_widget.set_visible(True)
        self.perform_layout()

    def draw_contents(self):
        # Keep movement consistent with framerate
        current_frame = time.time()
        self.delta_time = current_frame - self.last_frame
        self.last_frame = current_frame

        self.canvas_renderer.render()

    def keyboard_event(self, key, scancode, action, modifiers):
        if super(XStitchEditorApp, self).keyboard_event(key, scancode,
                                                        action, modifiers):
            return True

        if key == nanogui.glfw.KEY_ESCAPE and action == nanogui.glfw.PRESS:
            self.set_visible(False)
            return True

        camera_speed = 2.5 * self.delta_time

        if key == nanogui.glfw.KEY_LEFT:
            self.canvas_renderer.translation_x += camera_speed
        if key == nanogui.glfw.KEY_RIGHT:
            self.canvas_renderer.translation_x -= camera_speed
        if key == nanogui.glfw.KEY_UP:
            self.canvas_renderer.translation_y -= camera_speed
        if key == nanogui.glfw.KEY_DOWN:
            self.canvas_renderer.translation_y += camera_speed

        if key == nanogui.glfw.KEY_EQUAL and modifiers == nanogui.glfw.MOD_SHIFT:
            # TODO: center zoom on current screen center

            # 1) calculate position currently at center
            # 2) perform zoom
            # 3) find difference between new center and old center
            # 4) translate by this amount
            self.canvas_renderer.zoom += self.delta_time * 2
        if key == nanogui.glfw.KEY_MINUS and modifiers == nanogui.glfw.MOD_SHIFT:
            self.canvas_renderer.zoom -= self.delta_time * 2

        return False

    def scroll_event(self, mouse_position, delta):
        if super(XStitchEditorApp, self).scroll_event(mouse_position, delta):
            return True

        # We only care about vertical scroll
        if delta[1]:
            self.canvas_renderer.zoom += delta[1] * self.delta_time

        print(f"scroll event: {mouse_position}, {delta}")
        return False