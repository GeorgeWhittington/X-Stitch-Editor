import nanogui

from app import MANUFACTURERS


class DisabledButton(nanogui.Button):
    def mouse_button_event(self, *_):
        return False

    def mouse_enter_event(self, *_):
        return False


class ToolWindow(nanogui.Window):
    def __init__(self, parent):
        super(ToolWindow, self).__init__(parent, "Tools")
        self.parent = parent

        self.set_position((0, 0))
        self.set_layout(nanogui.GroupLayout(
            margin=5, spacing=5, group_spacing=10, group_indent=0))

        # Drawing Tools
        nanogui.Label(self, "Tools", "sans-bold")
        tool_wrapper = nanogui.Widget(self)
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
                parent.selected_tool = tool
                parent.set_cursor(cursor)
            return callback

        toolbutton = nanogui.ToolButton(tools, nanogui.icons.FA_HAND_PAPER)
        toolbutton.set_tooltip("Move")
        toolbutton.set_callback(make_tool_button_callback("MOVE", nanogui.Cursor.Hand))
        toolbutton.set_pushed(True)
        parent.selected_tool = "MOVE"
        parent.set_cursor(nanogui.Cursor.Hand)

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
        self.selected_thread_title = nanogui.Label(self, "Selected Thread", "sans-bold")

        self.selected_thread_widget = nanogui.Widget(self)
        self.selected_thread_widget.set_layout(nanogui.BoxLayout(
            orientation=nanogui.Orientation.Horizontal,
            alignment=nanogui.Alignment.Maximum, margin=0, spacing=5))

        self.selected_thread_button = DisabledButton(self.selected_thread_widget, "")
        self.selected_thread_label = nanogui.Label(self.selected_thread_widget, "")
        self.update_selected_thread_widget()

        nanogui.Label(self, "Palette", "sans-bold")

        v_scroll = nanogui.VScrollPanel(self)
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
                self.parent.selected_thread = thread
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

        self.set_visible(False)

    def update_selected_thread_widget(self):
        if self.parent.selected_thread is None:
            self.selected_thread_button.set_icon(nanogui.icons.FA_BAN)
            self.selected_thread_button.set_caption("")
        else:
            self.selected_thread_button.set_icon(0)
            self.selected_thread_label.set_caption(self.parent.selected_thread.number)
            self.selected_thread_button.set_background_color(nanogui.Color(*self.parent.selected_thread.colour))
            self.selected_thread_button.set_caption("  ")

    def mouse_over(self, x, y):
        """Returns True if the mouse is currently on top of this window"""
        control_x, control_y = self.absolute_position()
        control_width, control_height = self.size()

        return x >= control_x and y >= control_y and \
               x <= control_x + control_width and y <= control_y + control_height