#pragma once
#include <vector>
#include <map>
#include "threads.hpp"
#include "project.hpp"

struct RGBcolour {
    int R = -1;
    int G = -1;
    int B = -1;

    bool operator==(const RGBcolour& rhs) const {
        return R == rhs.R && G == rhs.G && B == rhs.B;
    };

    bool operator==(const Thread& rhs) const {
        return R == rhs.R && G == rhs.G && B == rhs.B;
    }

    bool operator<(const RGBcolour& rhs) const {
        return std::tie(R, G, B) < std::tie(rhs.R, rhs.G, rhs.B);
    }
};

class DitheringAlgorithm {
public:
    DitheringAlgorithm(std::vector<Thread*> *palette, int max_threads = INT_MAX) : _palette(palette), _max_threads(max_threads) {};

    // Finds the nearest colour from the available palette using a euclidian distance calculation
    // which is optimised using an internal cache
    Thread* find_nearest_neighbour(RGBcolour colour);

    virtual void dither(unsigned char *image, int width, int height, Project *project) = 0;

protected:
    int _max_threads;

private:
    std::map<RGBcolour, Thread*> _nearest_cache;
    // std::map<RGBcolour, Thread*> _nearest_cache;
    std::vector<Thread*> *_palette = nullptr;
};

class FloydSteinburg : DitheringAlgorithm {
public:
    FloydSteinburg(std::vector<Thread*> *palette, int max_threads = INT_MAX) : DitheringAlgorithm(palette, max_threads) {};

    virtual void dither(unsigned char *image, int width, int height, Project *project);
};

class Bayer : DitheringAlgorithm {
public:
    Bayer(std::vector<Thread*> *palette, int max_threads = INT_MAX) : DitheringAlgorithm(palette, max_threads) {};

    virtual void dither(unsigned char *image, int width, int height, Project *project);
};

class NoDither : DitheringAlgorithm {
public:
    NoDither(std::vector<Thread*> *palette, int max_threads = INT_MAX) : DitheringAlgorithm(palette, max_threads) {};

    virtual void dither(unsigned char *image, int width, int height, Project *project);
};