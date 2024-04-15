#pragma once
#include <nanogui/nanogui.h>

enum ToolOptions {
    NO_SELECTION,
    MOVE,
    SINGLE_STITCH,
    BACK_STITCH,
    ERASE,
    FILL,
    ZOOM_IN,
    ZOOM_OUT
};

enum ApplicationStates {
    LAUNCH,
    CREATE_PROJECT,
    CREATE_DITHERED_PROJECT,
    PROJECT_OPEN
};

const nanogui::Vector2i NO_STITCH_SELECTED = nanogui::Vector2i(-1, -1);
const nanogui::Vector2f NO_SUBSTITCH_SELECTED = nanogui::Vector2f(-1.f, -1.f);