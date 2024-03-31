#include "threads.hpp"

#include <string>
#include <map>

#include <tinyxml2.h>

// TODO: error handling, return false if anything fails?
// check file opened, check all attribute queries succeeded
bool load_manufacturer(const char *file_path, std::map<std::string, Thread*> *map) {
    tinyxml2::XMLDocument doc;
    doc.LoadFile(file_path);

    tinyxml2::XMLElement *element = doc.FirstChildElement("manufacturer");

    const char *manufacturer_name;
    element->QueryStringAttribute("name", &manufacturer_name);

    element = element->FirstChildElement("thread");
    const char *thread_number;
    const char *thread_description;
    int thread_r;
    int thread_g;
    int thread_b;

    while (true) {
        element->QueryStringAttribute("id", &thread_number);
        element->QueryStringAttribute("description", &thread_description);
        element->QueryIntAttribute("red", &thread_r);
        element->QueryIntAttribute("green", &thread_g);
        element->QueryIntAttribute("blue", &thread_b);

        Thread *thread_struct = new Thread;
        *thread_struct = {manufacturer_name, thread_number, thread_description, thread_r, thread_g, thread_b};

        (*map)[thread_number] = thread_struct;

        element = element->NextSiblingElement("thread");
        if (element == nullptr)
            break;
    }

    return true;
}