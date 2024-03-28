import nanogui


class MousePositionWindow(nanogui.Window):
    def __init__(self, parent):
        super(MousePositionWindow, self).__init__(parent, "")
        self.parent = parent

        self.set_layout(nanogui.GroupLayout(
            margin=5, spacing=5, group_spacing=0, group_indent=0))
        self.mouse_location_label = nanogui.Label(self, "")
        self.mouse_location_label_2 = nanogui.Label(self, "")
        self.set_visible(False)

    def set_captions(self, stitch_x, stitch_y, offset, colour = None):
        self.mouse_location_label.set_caption(f"stitch selected: {stitch_x}, {stitch_y}")
        if colour:
            self.mouse_location_label_2.set_caption(f"thread: {colour.company} {colour.number}")
            self.set_position((0, int(offset - 42)))
        else:
            self.mouse_location_label_2.set_caption("")
            self.set_position((0, int(offset - 26)))
        self.set_visible(True)