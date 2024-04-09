import nanogui

from app import Project, CanvasRenderer


class CreateNewProjectWindow(nanogui.Window):
    def __init__(self, parent):
        super(CreateNewProjectWindow, self).__init__(parent, "")
        self.parent = parent

        self.set_layout(nanogui.BoxLayout(
            nanogui.Orientation.Vertical, nanogui.Alignment.Middle,
            margin=15, spacing=5))

        errors = nanogui.Label(self, "")
        errors.set_color(nanogui.Color(204, 0, 0, 255))
        errors.set_visible(False)

        form_widget = nanogui.Widget(self)
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
                parent.perform_layout()
                return

            parent.switch_project(project)

            # reset form values
            title.set_value("")
            width.set_value(100)
            height.set_value(100)
            color.set_color(nanogui.Color(255, 255, 255, 255))

            self.set_visible(False)
            parent.tool_window.set_visible(True)
            parent.perform_layout()

        confirm_button = nanogui.Button(self, "Confirm")
        confirm_button.set_callback(create_new_project)

        self.center()
        self.set_visible(False)