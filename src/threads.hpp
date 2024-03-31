#pragma once
#include <string>
#include <map>
#include <nanogui/nanogui.h>

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

bool load_manufacturer(const char *file_path, std::map<std::string, Thread*> *map);