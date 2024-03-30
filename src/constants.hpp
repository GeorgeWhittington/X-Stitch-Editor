#include <string>
#include <nanogui/nanogui.h>
#pragma once

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
    LOAD_PROJECT,
    PROJECT_OPEN
};

struct Thread {
    std::string company;
    std::string number;
    std::string description;
    int R;
    int G;
    int B;

    nanogui::Color color() {
        return nanogui::Color(nanogui::Vector3i(R, G, B));
    };
};