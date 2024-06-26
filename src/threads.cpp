#include "threads.hpp"
#include "x_stitch_editor.hpp"

#include <sstream>
#include <string>
#include <map>
#include <fmt/core.h>

#include <tinyxml2.h>
#include <fmt/core.h>

bool is_duplicate(BlendedThread *t1, BlendedThread *t2) {
    return (t1->thread_1 == t2->thread_1 && t1->thread_2 == t2->thread_2) ||
           (t1->thread_1 == t2->thread_2 && t1->thread_2 == t2->thread_1);
}

// A simple average will give incorrect results, per: https://stackoverflow.com/a/29576746
BlendedThread create_blended_thread(SingleThread *thread_1, SingleThread *thread_2) {
    int R = ((thread_1->R * thread_1->R) + (thread_2->R * thread_2->R)) / 2;
    int G = ((thread_1->G * thread_1->G) + (thread_2->G * thread_2->G)) / 2;
    int B = ((thread_1->B * thread_1->B) + (thread_2->B * thread_2->B)) / 2;
    return BlendedThread(thread_1, thread_2, sqrt(R), sqrt(G), sqrt(B));
}

std::string load_manufacturer(const char *file_path, std::map<std::string, Thread*> *map) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(file_path) != tinyxml2::XML_SUCCESS)
        throw std::runtime_error(fmt::format("Failed to open {}", file_path));

    tinyxml2::XMLElement *element = doc.FirstChildElement("manufacturer");
    if (element == nullptr)
        throw std::runtime_error("Error parsing XML file: No manufacturer element found");

    const char *manufacturer_name;
    if (element->QueryStringAttribute("name", &manufacturer_name) != tinyxml2::XML_SUCCESS)
        throw std::runtime_error("Error parsing XML file: Manufacturer element has no name attribute");

    element = element->FirstChildElement("thread");
    if (element == nullptr)
        throw std::runtime_error("Error parsing XML file: No threads found");

    const char *thread_number;
    const char *thread_description;
    int thread_r;
    int thread_g;
    int thread_b;

    while (true) {
        if (element->QueryStringAttribute("id", &thread_number) != tinyxml2::XML_SUCCESS)
            throw std::runtime_error("Error parsing XML file: Thread element containing no id");
        if (element->QueryStringAttribute("description", &thread_description) != tinyxml2::XML_SUCCESS)
            throw std::runtime_error("Error parsing XML file: Thread element containing no description");
        if (element->QueryIntAttribute("red", &thread_r) != tinyxml2::XML_SUCCESS)
            throw std::runtime_error("Error parsing XML file: Thread element containing no red value");
        if (element->QueryIntAttribute("green", &thread_g) != tinyxml2::XML_SUCCESS)
            throw std::runtime_error("Error parsing XML file: Thread element containing no green value");
        if (element->QueryIntAttribute("blue", &thread_b) != tinyxml2::XML_SUCCESS)
            throw std::runtime_error("Error parsing XML file: Thread element containing no blue value");

        Thread *thread_struct = new SingleThread(manufacturer_name, thread_number, thread_description, thread_r, thread_g, thread_b);

        (*map)[thread_number] = thread_struct;

        element = element->NextSiblingElement("thread");
        if (element == nullptr)
            break;
    }

    return manufacturer_name;
}