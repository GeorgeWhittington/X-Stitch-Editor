#pragma once
#include <string>
#include <exception>
#include <regex>
#include <map>
#include <vector>
#include <nanogui/nanogui.h>
#include <tinyxml2.h>

using nanogui::Vector2i;
using nanogui::Vector2f;

std::string retrieve_string_attribute(tinyxml2::XMLElement *element, const char *key);

int retrieve_int_attribute(tinyxml2::XMLElement *element, const char *key);

nanogui::Color hex2rgb(std::string input);

int color_float_to_int(float color);

int index_3d(Vector2i stitch, int width);

struct Thread;
class XStitchEditorApplication;

struct BackStitch {
    Vector2f start;
    Vector2f end;
    int palette_index;

    BackStitch(Vector2f _start, Vector2f _end, int _palette_index) : start(_start), end(_end), palette_index(_palette_index) {};
};

struct Project
{
    std::string title;
    int width;
    int height;
    nanogui::Color bg_color;
    std::vector<Thread*> palette;

    std::vector<std::vector<int>> thread_data;
    std::shared_ptr<uint8_t[]> texture_data_array;
    std::vector<BackStitch> backstitches;

    std::string file_path;

    Project(std::string title_, int width_, int height_, nanogui::Color bg_color_);
    Project(const char *project_path, std::map<std::string, std::map<std::string, Thread*>*> *threads);
    void draw_stitch(Vector2i stitch, Thread *thread);
    void erase_stitch(Vector2i stitch);
    void fill_from_stitch(Vector2i stitch, Thread *thread);
    void draw_backstitch(Vector2f start_stitch, Vector2f end_stitch, Thread *thread);
    void sort_backstitches();
    Thread* find_thread_at_stitch(Vector2i stitch);
    void save(const char *filepath, XStitchEditorApplication *app);
    bool is_stitch_valid(Vector2i stitch);
    bool is_backstitch_valid(Vector2f stitch);
};
