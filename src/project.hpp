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
    // construct a project using a .OXS file.
    Project(const char *project_path, std::map<std::string, std::map<std::string, Thread*>*> *threads);
    // Draws a single stitch to the canvas. Throws std::runtime_error if the thread provided is not in the project palette.
    void draw_stitch(Vector2i stitch, Thread *thread);
    // Erases a single stitch from the canvas.
    void erase_stitch(Vector2i stitch);
    /* Fills an area with the thread specified.
    Throws std::runtime_error if the thread provided is not in the project palette. */
    void fill_from_stitch(Vector2i stitch, Thread *thread);
    // Draws a single backstitch to the canvas. Throws std::runtime_error if the thread provided is not in the project palette.
    void draw_backstitch(Vector2f start_stitch, Vector2f end_stitch, Thread *thread);
    // Sorts the backstitches according to their palette_index, high to low.
    void sort_backstitches();
    // Finds and combines backstitches with the same vector that connect.
    void collate_backstitches();
    // Returns the thread at the stitch provided (or nullptr if there are none).
    Thread* find_thread_at_stitch(Vector2i stitch);
    // Attempts to save the project to the file provided.
    void save(const char *filepath, XStitchEditorApplication *app);
    // Tests if a stitch is within the range for the canvas.
    bool is_stitch_valid(Vector2i stitch);
    // Tests if a backstitch stitch is within the range for the canvas.
    bool is_backstitch_valid(Vector2f stitch);
};
