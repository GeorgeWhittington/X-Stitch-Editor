class Project:
    def __init__(self, title, width, height, bg_color, palette):
        self.title = title.strip()
        self.width = width
        self.height = height
        self.bg_color = bg_color
        self.palette = palette

        if not self.title:
            raise ValueError("No project title provided")

        if self.width < 1 or self.height < 1:
            raise ValueError("Project dimensions cannot be smaller than 1 on either axis")