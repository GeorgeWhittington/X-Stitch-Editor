#include "project.hpp"
#include "threads.hpp"
#include "x_stitch_editor.hpp"
#include <fmt/core.h>
#include <iostream>

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

    // Allocate arrays
    thread_data = std::vector<std::vector<int>> (width, std::vector<int> (height, -1));
    texture_data_array = std::make_shared<uint8_t[]>(width * height * 4);

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
    texture_data_array[index] = 0;
    texture_data_array[index+1] = 0;
    texture_data_array[index+2] = 0;
    texture_data_array[index+3] = 0;
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

    doc.SaveFile(filepath);
}

bool Project::is_stitch_valid(Vector2i stitch) {
    return stitch[0] >= 0 && stitch[1] >= 0 && stitch[0] < width && stitch[1] < height;
}