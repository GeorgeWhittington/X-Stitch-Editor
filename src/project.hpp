#pragma once
#include <string>
#include <exception>
#include <regex>
#include <map>
#include <vector>
#include <nanogui/nanogui.h>
#include <tinyxml2.h>

std::string retrieve_string_attribute(tinyxml2::XMLElement *element, const char *key);

int retrieve_int_attribute(tinyxml2::XMLElement *element, const char *key);

nanogui::Color hex2rgb(std::string input);

int color_float_to_int(float color);

int index_3d(nanogui::Vector2i stitch, int width);

bool test_intersection(nanogui::Vector2f p0, nanogui::Vector2f p1, nanogui::Vector2f p2, nanogui::Vector2f p3);
nanogui::Vector2f* get_intersection(nanogui::Vector2f p0, nanogui::Vector2f p1, nanogui::Vector2f p2, nanogui::Vector2f p3);

struct Thread;
class XStitchEditorApplication;

struct BackStitch {
    nanogui::Vector2f start;
    nanogui::Vector2f end;
    int palette_index;

    BackStitch(nanogui::Vector2f _start, nanogui::Vector2f _end, int _palette_index) : start(_start), end(_end), palette_index(_palette_index) {};
};

class Project
{
public:
    std::string title;
    int width;
    int height;
    nanogui::Color bg_color;
    std::vector<Thread*> palette;

    std::vector<std::vector<int>> thread_data;
    std::shared_ptr<uint8_t[]> texture_data_array;
    std::vector<BackStitch> backstitches;

    std::string file_path;

    // construct an empty project (throws std::invalid_argument if title, width or height are invalid)
    Project(std::string title_, int width_, int height_, nanogui::Color bg_color_);
    // construct a project using a .OXS file.
    Project(const char *project_path, std::map<std::string, std::map<std::string, Thread*>*> *threads);
    ~Project();
    // Draws a single stitch to the canvas. Throws std::runtime_error if the thread provided is not in the project palette.
    void draw_stitch(nanogui::Vector2i stitch, Thread *thread);
    // Draws a single stitch to the canvas. Doesn't check if the thread provided is in the project palette.
    void draw_stitch(nanogui::Vector2i stitch, Thread *thread, int palette_index);
    // Erases a single stitch from the canvas.
    void erase_stitch(nanogui::Vector2i stitch);
    /* Fills an area with the thread specified.
    Throws std::runtime_error if the thread provided is not in the project palette. */
    void fill_from_stitch(nanogui::Vector2i stitch, Thread *thread);
    // Draws a single backstitch to the canvas. Throws std::runtime_error if the thread provided is not in the project palette.
    void draw_backstitch(nanogui::Vector2f start_stitch, nanogui::Vector2f end_stitch, Thread *thread);
    // Erases any backstitches that begin/end at the given stitch, or intersect it
    void erase_backstitches_intersecting(nanogui::Vector2i stitch);
    // Finds and combines backstitches with the same vector that connect.
    void collate_backstitches();
    // Returns the thread at the stitch provided (or nullptr if there are none).
    Thread* find_thread_at_stitch(nanogui::Vector2i stitch);
    // Attempts to save the project to the file provided.
    void save(const char *filepath, XStitchEditorApplication *app);
    // Tests if a stitch is within the range for the canvas.
    bool is_stitch_valid(nanogui::Vector2i stitch);
    // Tests if a backstitch stitch is within the range for the canvas.
    bool is_backstitch_valid(nanogui::Vector2f stitch);
};
