import gc

import nanogui
from objects import XStitchEditorApp

if __name__ == "__main__":
    nanogui.init()
    app = XStitchEditorApp()
    app.draw_all()
    app.set_visible(True)
    nanogui.mainloop(refresh=1 / 60.0 * 1000)

    del app
    gc.collect()
    nanogui.shutdown()