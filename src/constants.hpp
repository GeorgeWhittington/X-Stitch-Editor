#pragma once
#include <nanogui/nanogui.h>
#include <string>

#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

enum ToolOptions {
    NO_SELECTION,
    MOVE,
    SINGLE_STITCH,
    BACK_STITCH,
    ERASE,
    FILL
};

enum ApplicationStates {
    LAUNCH,
    CREATE_PROJECT,
    CREATE_PROJECT_FROM_IMAGE,
    PROJECT_OPEN
};

const nanogui::Vector2i NO_STITCH_SELECTED = nanogui::Vector2i(-1, -1);
const nanogui::Vector2f NO_SUBSTITCH_SELECTED = nanogui::Vector2f(-1.f, -1.f);
const nanogui::Color CANVAS_DEFAULT_COLOR = nanogui::Color(255, 255, 255, 255);

std::string get_resources_dir();