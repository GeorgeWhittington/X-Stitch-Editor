#include "dithering.hpp"
#include <set>
#include <nanogui/nanogui.h>
#include <iostream>

#define FS_ERR_RIGHT      0.4375f // 7/16
#define FS_ERR_DOWN_LEFT  0.1875f // 3/16
#define FS_ERR_DOWN       0.3125f // 5/16
#define FS_ERR_DOWN_RIGHT 0.0625f // 1/16
#define SQ_DIFF(x, y) (((x - y) * (x - y)) >> 2)

RGBcolour BLANK_COLOUR = RGBcolour{};

int square_diff(int x, int y) {
    int d = x - y;
    return (d * d) >> 2;
}

Thread* DitheringAlgorithm::find_nearest_neighbour(RGBcolour needle) {
    // First check cache
    Thread *match;
    try {
        match = _nearest_cache.at(needle);
        if (*match != BLANK_COLOUR)
            return match;
    } catch (std::out_of_range&) {}

    // Compare needle against all colours in palette to find closest.
    // The channels are very simply weighted according to their perceptual strength
    // in human vision. See: https://en.wikipedia.org/wiki/CIE_1931_color_space
    // The calculation is done with integers for speed, simpler fractional coef is next to each line
    int minimum_distance_sq = INT_MAX;
    for (Thread *colour : *_palette) {
        int distance_sq = (1063 * SQ_DIFF(needle.R, colour->R) / 5000) +  // 0.2126
                          (7152 * SQ_DIFF(needle.G, colour->G) / 10000) + // 0.7152
                          (361 * SQ_DIFF(needle.B, colour->B) / 5000);    // 0.0722

        if (distance_sq < minimum_distance_sq) {
            minimum_distance_sq = distance_sq;
            match = colour;
        }
    }

    // Save result in cache
    _nearest_cache[needle] = match;
    return match;
};

void apply_quant_error(int err_R, int err_G, int err_B, int *quant_error_ptr, int x, int y, int width, int height, float coefficient) {
    // check bounds
    if (x >= width || y >= height)
        return;

    // apply offset
    quant_error_ptr += 3 * INDEX(x, y, width);

    // add error
    *quant_error_ptr += err_R * coefficient;
    *(quant_error_ptr + 1) += err_G * coefficient;
    *(quant_error_ptr + 2) += err_B * coefficient;
}

void FloydSteinburg::dither(unsigned char *image, int width, int height, Project *project) {
    int *quant_error = new int[width * height * 3];
    for (int i = 0; i < width * height * 3; i++) {
        quant_error[i] = 0;
    }

    int i;
    int q_i;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            i = 4 * INDEX(x, y, width);
            q_i = 3 * INDEX(x, y, width);
            // It would be ideal to smartly handle opacity. Do not have time
            // to do this. Any areas that are 100% transparent are blank,
            // any other opacity level is treated as 100% opaque.
            if ((int)image[i+3] == 0)
                continue;

            // Clamp to 0..255
            RGBcolour old_pixel = RGBcolour{
                std::clamp(image[i] + quant_error[q_i], 0, 255),
                std::clamp(image[i+1] + quant_error[q_i+1], 0, 255),
                std::clamp(image[i+2] + quant_error[q_i+2], 0, 255)
            };
            Thread *new_pixel = find_nearest_neighbour(old_pixel);

            // Find the quantisation error and spread it across neighbouring pixels.
            int err_R = old_pixel.R - new_pixel->R;
            int err_G = old_pixel.G - new_pixel->G;
            int err_B = old_pixel.B - new_pixel->B;
            apply_quant_error(err_R, err_G, err_B, quant_error,
                              x + 1, y    , width, height, FS_ERR_RIGHT);
            apply_quant_error(err_R, err_G, err_B, quant_error,
                              x - 1, y + 1, width, height, FS_ERR_DOWN_LEFT);
            apply_quant_error(err_R, err_G, err_B, quant_error,
                              x    , y + 1, width, height, FS_ERR_DOWN);
            apply_quant_error(err_R, err_G, err_B, quant_error,
                              x + 1, y + 1, width, height, FS_ERR_DOWN_RIGHT);

            // Write to output image
            bool drawn = false;
            for (int j = 0; j < project->palette.size(); j++) {
                if (project->palette[j] == new_pixel) {
                    project->draw_stitch(nanogui::Vector2i(x, height - y - 1), new_pixel, j);
                    drawn = true;
                    break;
                }
            }
            if (!drawn) {
                project->palette.push_back(new_pixel);
                project->draw_stitch(nanogui::Vector2i(x, height - y - 1), new_pixel, project->palette.size() - 1);
            }
        }
    }

    delete[] quant_error;
}

void NoDither::dither(unsigned char *image, int width, int height, Project *project) {
    int i;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            i = 4 * INDEX(x, y, width);
            // It would be ideal to smartly handle opacity. Do not have time
            // to do this. Any areas that are 100% transparent are blank,
            // any other opacity level is treated as 100% opaque.
            if ((int)image[i+3] == 0)
                continue;

            Thread *new_pixel = find_nearest_neighbour(RGBcolour{image[i], image[i+1], image[i+2]});

            // Write to output image
            bool drawn = false;
            for (int j = 0; j < project->palette.size(); j++) {
                if (project->palette[j] == new_pixel) {
                    project->draw_stitch(nanogui::Vector2i(x, height - y - 1), new_pixel, j);
                    drawn = true;
                    break;
                }
            }
            if (!drawn) {
                project->palette.push_back(new_pixel);
                project->draw_stitch(nanogui::Vector2i(x, height - y - 1), new_pixel, project->palette.size() - 1);
            }
        }
    }
}