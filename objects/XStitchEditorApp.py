import nanogui

from objects import MANUFACTURERS, Thread


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
    def __init__(self):
        super(XStitchEditorApp, self).__init__((1024, 768), "X Stitch Editor")

        self.selected_thread = None
        self.selected_tool = None
        self.initialised = False

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

    def keyboard_event(self, key, scancode, action, modifiers):
        if super(XStitchEditorApp, self).keyboard_event(key, scancode,
                                                        action, modifiers):
            return True
        if key == nanogui.glfw.KEY_ESCAPE and action == nanogui.glfw.PRESS:
            self.set_visible(False)
            return True
        return False
