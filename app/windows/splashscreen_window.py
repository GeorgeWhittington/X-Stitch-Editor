import nanogui


class SplashscreenWindow(nanogui.Window):
    def __init__(self, parent):
        super(SplashscreenWindow, self).__init__(parent, "")
        self.parent = parent

        self.set_layout(nanogui.GroupLayout(margin=5, spacing=5, group_spacing=10, group_indent=0))

        def create_project():
            self.set_visible(False)
            parent.new_project_window.set_visible(True)
            parent.perform_layout()

        def convert_image():
            print("convert image")

        def load_project():
            print("load project")

        create_project_button = nanogui.Button(self, "Create new project")
        create_project_button.set_callback(create_project)
        create_project_button.set_font_size(30)

        convert_image_button = nanogui.Button(self, "Create new project from an image")
        convert_image_button.set_callback(convert_image)
        convert_image_button.set_font_size(30)

        load_project_button = nanogui.Button(self, "Load a project")
        load_project_button.set_callback(load_project)
        load_project_button.set_font_size(30)

        self.center()