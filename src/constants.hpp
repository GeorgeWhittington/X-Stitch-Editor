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