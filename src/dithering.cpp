#include "dithering.hpp"

RGBcolour BLANK_COLOUR = RGBcolour{};

RGBcolour DitheringAlgorithm::find_nearest_neighbour(RGBcolour needle) {
    // first check cache
    RGBcolour match = _nearest_cache.at(needle);
    if (match != BLANK_COLOUR)
        return match;

    // Compare needle against all colours in palette to find closest
    int minimum_distance_sq = INT_MAX;
    for (const RGBcolour colour : _palette) {
        int distance_sq = pow(needle.R - colour.R, 2) +
                          pow(needle.G - colour.G, 2) +
                          pow(needle.B - colour.B, 2);

        if (distance_sq < minimum_distance_sq) {
            minimum_distance_sq = distance_sq;
            match = colour;
        }
    }

    // Save result in cache
    _nearest_cache[needle] = match;
    return match;
};

void DitheringAlgorithm::set_palette(std::vector<RGBcolour> *palette) {
    _palette.clear();
    for (const RGBcolour colour : *palette) {
        _palette.push_back(colour);
    }
    _nearest_cache.clear(); // cache is invalid if palette changes
}

// void FloydSteinburg::dither() {

// }