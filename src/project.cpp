#include "project.hpp"
#include "threads.hpp"
#include "x_stitch_editor.hpp"
#include <fmt/core.h>
#include <iostream>
#include <queue>
#include <vector>
#include <set>

using nanogui::Vector2i;
using nanogui::Vector2f;

struct Vector2iCompare {
    bool operator() (const Vector2i& lhs, const Vector2i& rhs) const {
        return lhs[0] + lhs[1] < rhs[0] + rhs[1];
    }
};

std::string retrieve_string_attribute(tinyxml2::XMLElement *element, const char *key) {
    const char *string_attr;
    tinyxml2::XMLError err = element->QueryStringAttribute(key, &string_attr);
    if (err != tinyxml2::XML_SUCCESS)
        throw std::runtime_error("Error parsing file");

    return string_attr;
}

int retrieve_int_attribute(tinyxml2::XMLElement *element, const char *key) {
    int int_attr;
    tinyxml2::XMLError err = element->QueryIntAttribute(key, &int_attr);
    if (err != tinyxml2::XML_SUCCESS)
        throw std::runtime_error("Error parsing file");

    return int_attr;
}

float retrieve_float_attribute(tinyxml2::XMLElement *element, const char *key) {
    float float_attr;
    tinyxml2::XMLError err = element->QueryFloatAttribute(key, &float_attr);
    if (err != tinyxml2::XML_SUCCESS)
        throw std::runtime_error("Error parsing file");

    return float_attr;
}

nanogui::Color hex2rgb(std::string input) {
    if (input[0] == '#')
        input.erase(0, 1);

    if (input.length() != 6)
        throw std::invalid_argument("Hex string must be in format '#FFFFFF' or 'FFFFFF'");

    unsigned long value = stoul(input, nullptr, 16);

    int r = (value >> 16) & 0xff;
    int g = (value >> 8) & 0xff;
    int b = (value >> 0) & 0xff;

    return nanogui::Color(r, g, b, 255);
}

int color_float_to_int(float color) {
    return std::max(0, std::min(255, (int)floor(color * 256.0)));
}

int index_3d(Vector2i stitch, int width) {
    return stitch[1] * width * 4 + stitch[0] * 4;
}

Project::Project(std::string title_, int width_, int height_, nanogui::Color bg_color_)
: bg_color(bg_color_)
{
    if (title_ == "")
        throw std::invalid_argument("No project title provided");

    if (width_ < 1 || height_ < 1)
        throw std::invalid_argument("Project dimensions cannot be smaller than 1 on either axis");

    title = title_;
    width = width_;
    height = height_;

    thread_data = std::vector<std::vector<int>> (width, std::vector<int> (height, -1));
    texture_data_array = std::make_shared<uint8_t[]>(width * height * 4);

    for (int i = 0; i < width * height * 4; i++) {
        texture_data_array[i] = 255;
    }
}

Project::Project(const char *project_path, std::map<std::string, std::map<std::string, Thread*>*> *threads) {
    using namespace tinyxml2;

    file_path = project_path;

    XMLDocument doc;
    doc.LoadFile(project_path);
    XMLElement *chart = doc.FirstChildElement("chart");

    XMLElement *properties = chart->FirstChildElement("properties");

    title = retrieve_string_attribute(properties, "charttitle");
    width = retrieve_int_attribute(properties, "chartwidth");
    height = retrieve_int_attribute(properties, "chartheight");
    // TODO: consider also retrieving author + copyright attrs?

    if (std::all_of(title.begin(), title.end(), isspace))
        title = "Untitled";

    // Allocate arrays
    thread_data = std::vector<std::vector<int>> (width, std::vector<int> (height, -1));
    texture_data_array = std::make_shared<uint8_t[]>(width * height * 4);

    for (int i = 0; i < width * height * 4; i++) {
        texture_data_array[i] = 255;
    }

    int palette_length = retrieve_int_attribute(properties, "palettecount");

    XMLElement *palette_element = chart->FirstChildElement("palette");
    XMLElement *cloth = palette_element->FirstChildElement("palette_item");
    XMLElement *palette_item = cloth;

    std::string cloth_color_str = retrieve_string_attribute(cloth, "color");
    bg_color = hex2rgb(cloth_color_str);

    std::regex thread_regex = std::regex("([a-zA-Z0-9]+) +([a-zA-Z0-9]+)");
    bool match = true;
    for (int i = 0; i < palette_length; i++) {
        palette_item = palette_item->NextSiblingElement("palette_item");
        std::string thread_str = (retrieve_string_attribute(palette_item, "number")).c_str();
        std::smatch matches;
        match = std::regex_match(thread_str, matches, thread_regex, std::regex_constants::match_default);

        if (!match)
            throw std::runtime_error("Error parsing file, palette number in unrecognised format");

        try {
            auto manufacturer = threads->at(matches.str(1));
            palette.push_back(manufacturer->at(matches.str(2)));
        } catch (std::out_of_range&) {
            throw std::runtime_error(fmt::format("Error parsing file, unrecognised thread referenced: {} {}", matches.str(1), matches.str(2)));
        }
    }

    XMLElement *stitch = chart->FirstChildElement("fullstitches");
    if (stitch != nullptr) {
        stitch = stitch->FirstChildElement("stitch");

        if (stitch != nullptr) {
            while (true) {
                // get data out
                int x = retrieve_int_attribute(stitch, "x");
                int y = retrieve_int_attribute(stitch, "y");
                int index = retrieve_int_attribute(stitch, "palindex");

                Thread *thread = palette[index - 1];

                draw_stitch(Vector2i(x, (height - y)), thread);

                stitch = stitch->NextSiblingElement("stitch");
                if (stitch == nullptr)
                    break;
            }
        }
    }

    // TODO: Read part-stitch data

    XMLElement *backstitch = chart->FirstChildElement("backstitches");
    if (backstitch != nullptr) {
        backstitch = backstitch->FirstChildElement("backstitch");

        if (backstitch != nullptr) {
            while (true) {
                float x1 = retrieve_float_attribute(backstitch, "x1");
                float y1 = retrieve_float_attribute(backstitch, "y1");
                float x2 = retrieve_float_attribute(backstitch, "x2");
                float y2 = retrieve_float_attribute(backstitch, "y2");
                int index = retrieve_int_attribute(backstitch, "palindex");
                // Objecttype attribute which can mean that some stitches are not backstitches
                // but instead something complex like a daisy. Will ignore this and just render them
                // as backstitches.

                // There's a sequence attr also, but I don't render backstitches in such a way
                // that the direction they are drawn in matters, so I'm ignoring it.

                float width_f = (float)width;
                float height_f = (float)height;

                // clamp to 0..width/height
                x1 = std::max(0.f, std::min(width_f, x1));
                x2 = std::max(0.f, std::min(width_f, x2));
                y1 = std::max(0.f, std::min(height_f, (height_f - y1 + 1)));
                y2 = std::max(0.f, std::min(height_f, (height_f - y2 + 1)));

                // round to whole or 0.5 increments
                x1 = std::round(x1 * 2.f) / 2.f;
                x2 = std::round(x2 * 2.f) / 2.f;
                y1 = std::round(y1 * 2.f) / 2.f;
                y2 = std::round(y2 * 2.f) / 2.f;

                Thread *thread = palette[index - 1];

                draw_backstitch(Vector2f(x1, y1), Vector2f(x2, y2), thread);

                backstitch = backstitch->NextSiblingElement("backstitch");
                if (backstitch == nullptr)
                    break;
            }
            collate_backstitches();
        }
    }
};

void Project::draw_stitch(Vector2i stitch, Thread *thread) {
    int palette_index = -1;
    for (int i = 0; i < palette.size(); i++) {
        if (palette[i] == thread) {
            palette_index = i;
            break;
        }
    }

    if (palette_index == -1)
        throw std::runtime_error("Thread provided is not in this project's palette");

    thread_data[stitch[0]][stitch[1]] = palette_index;

    int index = index_3d(stitch, width);
    texture_data_array[index] = thread->R;
    texture_data_array[index+1] = thread->G;
    texture_data_array[index+2] = thread->B;
    texture_data_array[index+3] = 255;
}

void Project::erase_stitch(Vector2i stitch) {
    thread_data[stitch[0]][stitch[1]] = -1;

    int index = index_3d(stitch, width);
    texture_data_array[index] = 255;
    texture_data_array[index+1] = 255;
    texture_data_array[index+2] = 255;
    texture_data_array[index+3] = 255;
}

void Project::fill_from_stitch(Vector2i stitch, Thread *thread) {
    Thread *target_thread = find_thread_at_stitch(stitch);

    if (target_thread == thread)
        return;

    std::queue<Vector2i> unvisited;
    std::set<Vector2i, Vector2iCompare> visited;

    unvisited.push(stitch);

    while(unvisited.size() > 0) {
        Vector2i current = unvisited.front();
        Thread *current_thread = find_thread_at_stitch(current);

        if (current_thread != target_thread) {
            // not part of fill area, don't change this or look at its surrounding pixels
            unvisited.pop();
            continue;
        }

        Vector2i touching[4] = {
            {current[0] - 1, current[1]},
            {current[0] + 1, current[1]},
            {current[0], current[1] + 1},
            {current[0], current[1] - 1}
        };

        for (const Vector2i& s : touching) {
            if (is_stitch_valid(s) && !visited.contains(s))
                unvisited.push(s);
        }

        draw_stitch(current, thread);

        unvisited.pop();
    }
}

void Project::draw_backstitch(Vector2f start_stitch, Vector2f end_stitch, Thread *thread) {
    // Check if stitch is in range
    if (!is_backstitch_valid(start_stitch) || !(is_backstitch_valid(end_stitch)))
        return;

    // if a backstitch already exists at this position, delete it
    for (auto itr = backstitches.begin(); itr != backstitches.end(); itr++) {
        BackStitch bs = *itr;
        if (bs.start == start_stitch && bs.end == end_stitch ||
            bs.start == end_stitch && bs.end == start_stitch) {
            backstitches.erase(itr);
            break;
        }
    }

    int palette_index = -1;
    for (int i = 0; i < palette.size(); i++) {
        if (palette[i] == thread) {
            palette_index = i;
            break;
        }
    }

    if (palette_index == -1)
        throw std::runtime_error("Thread provided is not in this project's palette");

    backstitches.push_back(BackStitch(start_stitch, end_stitch, palette_index));
}

// Algorithm from: https://stackoverflow.com/a/1968345
bool test_intersection(Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3) {
    float s1_x, s1_y, s2_x, s2_y;
    s1_x = p1[0] - p0[0];
    s1_y = p1[1] - p0[1];
    s2_x = p3[0] - p2[0];
    s2_y = p3[1] - p2[1];

    float s, t;
    s = (-s1_y * (p0[0] - p2[0]) + s1_x * (p0[1] - p2[1])) / (-s2_x * s1_y + s1_x * s2_y);
    t = ( s2_x * (p0[1] - p2[1]) - s2_y * (p0[0] - p2[0])) / (-s2_x * s1_y + s1_x * s2_y);

    return s >= 0 && s <= 1 && t >= 0 && t <= 1;
}

// Algorithm from: https://stackoverflow.com/a/1968345
Vector2f* get_intersection(Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3) {
    float s1_x, s1_y, s2_x, s2_y;
    s1_x = p1[0] - p0[0];
    s1_y = p1[1] - p0[1];
    s2_x = p3[0] - p2[0];
    s2_y = p3[1] - p2[1];

    float s, t;
    s = (-s1_y * (p0[0] - p2[0]) + s1_x * (p0[1] - p2[1])) / (-s2_x * s1_y + s1_x * s2_y);
    t = ( s2_x * (p0[1] - p2[1]) - s2_y * (p0[0] - p2[0])) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
        float i_x = p0[0] + (t * s1_x);
        float i_y = p0[1] + (t * s1_y);
        return new Vector2f(i_x, i_y);
    }

    return nullptr;
}

void Project::erase_backstitches_intersecting(Vector2i stitch) {
    std::vector<int> to_delete;

    Vector2f substitches[9] = {
        Vector2f(stitch),
        Vector2f(stitch[0] + 0.5f, stitch[1]),
        Vector2f(stitch[0] + 1.f, stitch[1]),
        Vector2f(stitch[0], stitch[1] + 0.5f),
        Vector2f(stitch[0] + 0.5f, stitch[1] + 0.5f),
        Vector2f(stitch[0] + 1.f, stitch[1] + 0.5f),
        Vector2f(stitch[0], stitch[1] + 1.f),
        Vector2f(stitch[0] + 0.5f, stitch[1] + 1.f),
        Vector2f(stitch[0] + 1.f, stitch[1] + 1.f),
    };

    std::pair<Vector2f, Vector2f> stitch_edges[4] = {
        std::pair(substitches[6], substitches[8]), // top
        std::pair(substitches[0], substitches[2]), // bottom
        std::pair(substitches[0], substitches[6]), // left
        std::pair(substitches[8], substitches[2])  // right
    };

    for (int i = 0; i < backstitches.size(); i++) {
        BackStitch bs = backstitches[i];

        for (Vector2f ss : substitches) {
            if (bs.end == ss || bs.start == ss) {
                to_delete.push_back(i);
                goto deleted;
            }
        }

        for (std::pair<Vector2f, Vector2f> edge : stitch_edges) {
            if (test_intersection(bs.start, bs.end, edge.first, edge.second)) {
                to_delete.push_back(i);
                goto deleted;
            }
        }
deleted: // continue to check next backstitch
    continue;
    }

    // Delete all intersecting backstitches
    for (auto rit = to_delete.rbegin(); rit != to_delete.rend(); rit++) {
        backstitches.erase(backstitches.begin() + *rit);
    }
}

void Project::collate_backstitches() {
    std::vector<BackStitch> new_backstitches;

    // Split backstitches up by colour
    std::map<float, std::vector<BackStitch>> backstitches_split[palette.size()];
    for (BackStitch bs : backstitches) {
        float gradient;
        if (bs.end[0] - bs.start[0] != 0) {
            gradient = (bs.end[1] - bs.start[1]) / (bs.end[0] - bs.start[0]);
        } else {
            gradient = INT_MAX;
        }

        backstitches_split[bs.palette_index][gradient].push_back(bs);
    }

    for (std::map<float, std::vector<BackStitch>> bs_map : backstitches_split) {
        for (auto const& [key, vec] : bs_map) {
            // Only a single backstitch with the same colour+gradient
            if (vec.size() == 1) {
                new_backstitches.push_back(vec.at(0));
                continue;
            }

            std::vector<BackStitch> editable_vec = vec;

            while (true) {
                std::vector<BackStitch> combined;
                std::set<int> removed;

                for (int i = 0; i < editable_vec.size(); i++) {
                    if (removed.find(i) != removed.end())
                        continue;

                    for (int j = 0; j < editable_vec.size(); j++) {
                        if (i == j || removed.find(j) != removed.end())
                            continue;

                        BackStitch bs1 = editable_vec.at(i);
                        BackStitch bs2 = editable_vec.at(j);

                        if (bs1.start == bs2.start) {
                            combined.push_back(BackStitch(bs1.end, bs2.end, bs1.palette_index));
                        } else if (bs1.start == bs2.end) {
                            combined.push_back(BackStitch(bs1.end, bs2.start, bs1.palette_index));
                        } else if (bs1.end == bs2.start) {
                            combined.push_back(BackStitch(bs1.start, bs2.end, bs1.palette_index));
                        } else if (bs1.end == bs2.end) {
                            combined.push_back(BackStitch(bs1.start, bs2.start, bs1.palette_index));
                        } else {
                            continue;
                        }

                        removed.insert(i);
                        removed.insert(j);
                    }
                }

                // Delete items that have been combined (in high to low index, to preserve the order)
                std::set<int>::reverse_iterator rit;
                for (auto rit = removed.rbegin(); rit != removed.rend(); rit++) {
                    editable_vec.erase(editable_vec.begin() + *rit);
                }

                // No combinations made, copy modified backstitches and exit while loop
                if (combined.size() == 0) {
                    for (BackStitch bs : editable_vec) {
                        new_backstitches.push_back(bs);
                    }
                    break;
                }

                for (BackStitch bs : combined) {
                    editable_vec.push_back(bs);
                }
            }
        }
    }

    backstitches = new_backstitches;
}

Thread* Project::find_thread_at_stitch(Vector2i stitch) {
    int palette_id = thread_data[stitch[0]][stitch[1]];
    if (palette_id == -1)
        return nullptr;

    try {
        Thread *t = palette.at(palette_id);
        return t;
    } catch (std::out_of_range&) {
        // this shouldn't happen, but to be safe clear the stitch
        // that contains a thread we don't know
        erase_stitch(stitch);
        return nullptr;
    }
}

void Project::save(const char *filepath, XStitchEditorApplication *app) {
    using namespace tinyxml2;

    XMLDocument doc(false);

    XMLDeclaration *decl_element = doc.NewDeclaration("xml version='1.0' encoding='UTF-8'");
    doc.InsertEndChild(decl_element);

    XMLElement *chart_element = doc.NewElement("chart");
    doc.InsertEndChild(chart_element);

    XMLElement *format_element = chart_element->InsertNewChildElement("format");
    format_element->SetAttribute("comments01", "Designed to allow interchange of basic pattern data between any cross stitch style software");
    format_element->SetAttribute("comments02", "the 'properties' section establishes size, copyright, authorship and software used");
    format_element->SetAttribute("comments03", "The features of each software package varies, but using XML each can pick out the things it can deal with, while ignoring others");
    format_element->SetAttribute("comments04", "The basic items are :");
    format_element->SetAttribute("comments05", "'palette'..a set of colors used in the design: palettecount excludes cloth color, which is item 0");
    format_element->SetAttribute("comments06", "'fullstitches'.. simple crosses");
    format_element->SetAttribute("comments07", "'backstitches'.. lines/objects with a start and end point");
    format_element->SetAttribute("comments08", "(There is a wide variety of ways of treating part stitches, knots, beads and so on.)");
    format_element->SetAttribute("comments09", "Colors are expressed in hex RGB format.");
    format_element->SetAttribute("comments10", "Decimal numbers use US/UK format where '.' is the indicator - eg 0.5 is 'half'");
    format_element->SetAttribute("comments11", "For readability, please use words not enumerations");
    format_element->SetAttribute("comments12", "The properties, fullstitches, and backstitches elements should be considered mandatory, even if empty");
    format_element->SetAttribute("comments13", "element and attribute names are always lowercase");

    XMLElement *properties_element = chart_element->InsertNewChildElement("properties");
    properties_element->SetAttribute("oxsversion", 1.f);
    properties_element->SetAttribute("software", "X Stitch Editor");
    properties_element->SetAttribute("software_version", 0.1f);
    properties_element->SetAttribute("chartheight", height);
    properties_element->SetAttribute("chartwidth", width);
    properties_element->SetAttribute("charttitle", title.c_str());
    properties_element->SetAttribute("author", "");
    properties_element->SetAttribute("copyright", "");
    properties_element->SetAttribute("palettecount", (int)palette.size());

    XMLElement *palette_element = chart_element->InsertNewChildElement("palette");

    XMLElement *palette_item_element = palette_element->InsertNewChildElement("palette_item");
    palette_item_element->SetAttribute("index", 0);
    palette_item_element->SetAttribute("number", "cloth");
    palette_item_element->SetAttribute("name", "cloth");
    palette_item_element->SetAttribute("color", fmt::format("{:02x}{:02x}{:02x}",
        color_float_to_int(bg_color.r()),
        color_float_to_int(bg_color.g()),
        color_float_to_int(bg_color.b())).c_str());

    for (int i = 0; i < palette.size(); i++) {
        palette_item_element = palette_element->InsertNewChildElement("palette_item");
        palette_item_element->SetAttribute("index", i + 1);
        palette_item_element->SetAttribute("number", fmt::format("{} {}", palette[i]->company, palette[i]->number).c_str());
        palette_item_element->SetAttribute("name", palette[i]->description.c_str());
        palette_item_element->SetAttribute("color", fmt::format("{:02x}{:02x}{:02x}", palette[i]->R, palette[i]->G, palette[i]->B).c_str());
    }

    XMLElement *full_stitches_element = chart_element->InsertNewChildElement("fullstitches");
    XMLElement *stitch_element;

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int palette_index = thread_data[x][y];
            if (palette_index == -1)
                continue; // blank stitch

            stitch_element = full_stitches_element->InsertNewChildElement("stitch");
            stitch_element->SetAttribute("x", x);
            stitch_element->SetAttribute("y", height - y);
            stitch_element->SetAttribute("palindex", palette_index + 1);
        }
    }

    XMLElement *back_stitches_element = chart_element->InsertNewChildElement("backstitches");
    XMLElement *backstitch_element;

    collate_backstitches();
    for (BackStitch bs : backstitches) {
        backstitch_element = back_stitches_element->InsertNewChildElement("backstitch");
        backstitch_element->SetAttribute("x1", bs.start[0]);
        backstitch_element->SetAttribute("y1", (float)height - bs.start[1] + 1);
        backstitch_element->SetAttribute("x2", bs.end[0]);
        backstitch_element->SetAttribute("y2", (float)height - bs.end[1] + 1);
        backstitch_element->SetAttribute("palindex", bs.palette_index + 1);
        backstitch_element->SetAttribute("objecttype", "backstitch");
        backstitch_element->SetAttribute("sequence", 0);
    }

    doc.SaveFile(filepath);
}

bool Project::is_stitch_valid(Vector2i stitch) {
    return stitch[0] >= 0 && stitch[1] >= 0 && stitch[0] < width && stitch[1] < height;
}

bool Project::is_backstitch_valid(Vector2f stitch) {
    return stitch[0] >= 0.f && stitch[0] <= (float)width &&
           stitch[1] >= 0.f && stitch[1] <= (float)height;
}