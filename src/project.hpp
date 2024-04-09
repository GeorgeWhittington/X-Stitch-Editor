#pragma once
#include <string>
#include <exception>
#include <nanogui/nanogui.h>

struct Project
{
    std::string title;
    int width;
    int height;
    nanogui::Color bg_color;
    // palette (vector of thread ids? so vector of strings)

    Project(std::string title_, int width_, int height_, nanogui::Color bg_color_)
    : bg_color(bg_color_)
    {
        if (title_ == "")
            throw std::invalid_argument("No project title provided");

        if (width_ < 1 || height_ < 1)
            throw std::invalid_argument("Project dimensions cannot be smaller than 1 on either axis");

        title = title_;
        width = width_;
        height = height_;
    };
};
