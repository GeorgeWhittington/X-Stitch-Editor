#pragma once
#include <vector>
#include <map>
#include "threads.hpp"
#include "project.hpp"

#define INDEX(x, y, width) (x + (width * y))
#define THRESHOLD_COLOUR 0.64f
#define THRESHOLD_GREYSCALE 1.f

enum BayerOrders {
    TWO = 2U,
    FOUR = 4U,
    EIGHT = 8U,
    SIXTEEN = 16U
};

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
    DitheringAlgorithm(std::vector<Thread*> *palette, int max_threads = INT_MAX, bool blend_threads = false)
    : _palette(palette), _blend_threads(blend_threads)
    {
        _max_threads = max_threads <= 0 ? 1 : max_threads;
    };

    // Finds the nearest colour from the available palette using a euclidian distance calculation
    // which is optimised using an internal cache
    Thread* find_nearest_neighbour(RGBcolour colour, std::vector<Thread*> *palette);

protected:
    int _max_threads;
    std::vector<Thread*> *_palette = nullptr;
    bool _blend_threads;

    void set_palette(std::vector<Thread*> *new_palette);
    void reduce_palette(unsigned char *image, int width, int height, std::vector<Thread*> *new_palette, bool median_cut_floor = false);
    void draw_stitch(int x, int y, int height, Thread *new_pixel, Project *project);
    void expand_palette(std::vector<Thread*> *new_palette);

private:
    std::map<RGBcolour, Thread*> _nearest_cache;

    void median_cut(std::vector<RGBcolour> *image, int depth, std::vector<RGBcolour> *points);
    void create_closest_palette(std::vector<RGBcolour> CIELuv_averages, std::vector<Thread*> *new_palette);
};

class FloydSteinburg : DitheringAlgorithm {
public:
    FloydSteinburg(std::vector<Thread*> *palette, int max_threads = INT_MAX, bool blend_threads = false) : DitheringAlgorithm(palette, max_threads, blend_threads) {};

    void dither(unsigned char *image, int width, int height, Project *project);

private:
    void apply_quant_error(int err_R, int err_G, int err_B, int *quant_error_ptr, int x, int y, int width, int height, float coefficient);
};

template <uint ORDER = 4U>
class Bayer : DitheringAlgorithm {
public:
    Bayer(std::vector<Thread*> *palette, int max_threads = INT_MAX, bool blend_threads = false) : DitheringAlgorithm(palette, max_threads, blend_threads) {
        int BAYER2x2[2][2] = {
            {0, 2},
            {3, 1}
        };
        int BAYER4x4[4][4] = {
            { 0,  8,  2, 10},
            {12,  4, 14,  6},
            { 3, 11,  1,  9},
            {15,  7, 13,  5}
        };
        int BAYER8x8[8][8] = {
            { 0, 32,  8, 40,  2, 34, 10, 42},
            {48, 16, 56, 24, 50, 18, 58, 26},
            {12, 44,  4, 36, 14, 46,  6, 38},
            {60, 28, 52, 20, 62, 30, 54, 22},
            { 3, 35, 11, 43,  1, 33,  9, 41},
            {51, 19, 59, 27, 49, 17, 57, 25},
            {15, 47,  7, 39, 13, 45,  5, 37},
            {63, 31, 55, 23, 61, 29, 53, 21}
        };
        int BAYER16x16[16][16] = {
            {  0, 191,  48, 239,  12, 203,  60, 251,   3, 194,  51, 242,  15, 206,  63, 254},
            {127,  64, 175, 112, 139,  76, 187, 124, 130,  67, 178, 115, 142,  79, 190, 127},
            { 32, 223,  16, 207,  44, 235,  28, 219,  35, 226,  19, 210,  47, 238,  31, 222},
            {159,  96, 143,  80, 171, 108, 155,  92, 162,  99, 146,  83, 174, 111, 158,  95},
            {  8, 199,  56, 247,   4, 195,  52, 243,  11, 202,  59, 250,   7, 198,  55, 246},
            {135,  72, 183, 120, 131,  68, 179, 116, 138,  75, 186, 123, 134,  71, 182, 119},
            { 40, 231,  24, 215,  36, 227,  20, 211,  43, 234,  27, 218,  39, 230,  23, 214},
            {167, 104, 151,  88, 163, 100, 147,  84, 170, 107, 154,  91, 166, 103, 150,  87},
            {  2, 193,  50, 241,  14, 205,  62, 253,   1, 192,  49, 240,  13, 204,  61, 252},
            {129,  66, 177, 114, 141,  78, 189, 126, 128,  65, 176, 113, 140,  77, 188, 125},
            { 34, 225,  18, 209,  46, 237,  30, 221,  33, 224,  17, 208,  45, 236,  29, 220},
            {161,  98, 145,  82, 173, 110, 157,  94, 160,  97, 144,  81, 172, 109, 156,  93},
            { 10, 201,  58, 249,   6, 197,  54, 245,   9, 200,  57, 248,   5, 196,  53, 244},
            {137,  74, 185, 122, 133,  70, 181, 118, 136,  73, 184, 121, 132,  69, 180, 117},
            { 42, 233,  26, 217,  38, 229,  22, 213,  41, 232,  25, 216,  37, 228,  21, 212},
            {169, 106, 153,  90, 165, 102, 149,  86, 168, 105, 152,  89, 164, 101, 148,  85}
        };

        switch (ORDER) {
        case BayerOrders::TWO:
            for (int i = 0; i < ORDER; i++) {
                for (int j = 0; j < ORDER; j++) {
                    _matrix[i][j] = THRESHOLD_COLOUR * BAYER2x2[i][j];
                }
            }
            break;
        case BayerOrders::FOUR:
            for (int i = 0; i < ORDER; i++) {
                for (int j = 0; j < ORDER; j++) {
                    _matrix[i][j] = THRESHOLD_COLOUR * BAYER4x4[i][j];
                }
            }
            break;
        case BayerOrders::EIGHT:
            for (int i = 0; i < ORDER; i++) {
                for (int j = 0; j < ORDER; j++) {
                    _matrix[i][j] = THRESHOLD_COLOUR * BAYER8x8[i][j];
                }
            }
            break;
        case BayerOrders::SIXTEEN:
            for (int i = 0; i < ORDER; i++) {
                for (int j = 0; j < ORDER; j++) {
                    _matrix[i][j] = THRESHOLD_COLOUR * BAYER16x16[i][j];
                }
            }
            break;

        default:
            throw std::invalid_argument("ORDER must be either 2, 4, 8 or 16");
            break;
        }
    };

    void dither(unsigned char *image, int width, int height, Project *project);

private:
    int _matrix[ORDER][ORDER];
};

template<uint ORDER>
void Bayer<ORDER>::dither(unsigned char *image, int width, int height, Project *project) {
    std::vector<Thread*> new_palette;
    if (_palette->size() > _max_threads) {
        reduce_palette(image, width, height, &new_palette);
    }

    std::vector<Thread*> new_new_palette;
    if (_blend_threads) {
        expand_palette(&new_new_palette);
    }

    int i, row, factor;
    for (int y = 0; y < height; y++) {
        row = y & ORDER - 1;
        for (int x = 0; x < width; x++) {
            i = 4 * INDEX(x, y, width);
            // It would be ideal to smartly handle opacity. Do not have time
            // to do this. Any areas that are 100% transparent are blank,
            // any other opacity level is treated as 100% opaque.
            if ((int)image[i+3] == 0)
                continue;

            // TODO: look into normalising threshold matrix, when order is
            // high the brightness is really bad
            factor = _matrix[x & ORDER - 1][row];
            Thread *new_pixel = find_nearest_neighbour(RGBcolour{
                std::clamp(image[i] + factor, 0, 255), std::clamp(image[i+1] + factor, 0, 255), std::clamp(image[i+2] + factor, 0, 255)
            }, nullptr);

            draw_stitch(x, y, height, new_pixel, project);
        }
    }
}

class NoDither : DitheringAlgorithm {
public:
    NoDither(std::vector<Thread*> *palette, int max_threads = INT_MAX, bool blend_threads = false) : DitheringAlgorithm(palette, max_threads, blend_threads) {};

    void dither(unsigned char *image, int width, int height, Project *project);
};