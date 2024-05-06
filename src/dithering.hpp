#pragma once
#include <vector>
#include <map>

struct RGBcolour {
    int R = -1;
    int G = -1;
    int B = -1;

    bool operator==(const RGBcolour& rhs) const {
        return R == rhs.R && G == rhs.G && B == rhs.B;
    };

    bool operator<(const RGBcolour& rhs) const {
        return R + G + B < rhs.R + rhs.G + rhs.B;
    }
};

class DitheringAlgorithm {
public:
    DitheringAlgorithm(std::vector<RGBcolour> *palette) : _palette(*palette) {};

    // Finds the nearest colour from the available palette using a euclidian distance calculation
    // which is optimised using an internal cache
    RGBcolour find_nearest_neighbour(RGBcolour colour);

    void set_palette(std::vector<RGBcolour> *palette);

    virtual void dither();

private:
    std::map<RGBcolour, RGBcolour> _nearest_cache;
    std::vector<RGBcolour> _palette;
};

class FloydSteinburg : DitheringAlgorithm {
public:
    FloydSteinburg(std::vector<RGBcolour> *palette) : DitheringAlgorithm(palette) {};

    virtual void dither();
};

class Bayer : DitheringAlgorithm {
public:
    Bayer(std::vector<RGBcolour> *palette) : DitheringAlgorithm(palette) {};

    virtual void dither();
};

// for bayer, you just apply the algorithm you would apply in the 1 bit greyscale case
// to all three channels. Would assume apprx the same approach for floyd steinburg.