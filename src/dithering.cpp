#include "dithering.hpp"
#include <set>
#include <nanogui/nanogui.h>

#define INDEX(x, y, width) (x + (width * y))
#define FS_ERR_RIGHT      0.4375f // 7/16
#define FS_ERR_DOWN_LEFT  0.1875f // 3/16
#define FS_ERR_DOWN       0.3125f // 5/16
#define FS_ERR_DOWN_RIGHT 0.0625f // 1/16

RGBcolour BLANK_COLOUR = RGBcolour{};

Thread* DitheringAlgorithm::find_nearest_neighbour(RGBcolour needle) {
    // Clamp input to 0..255
    needle = RGBcolour{std::clamp(needle.R, 0, 255),
                       std::clamp(needle.G, 0, 255),
                       std::clamp(needle.B, 0, 255)};

    // First check cache
    Thread *match;
    try {
        match = _nearest_cache.at(needle);
        if (*match != BLANK_COLOUR)
            return match;
    } catch (std::out_of_range&) {}

    // Compare needle against all colours in palette to find closest
    int minimum_distance_sq = INT_MAX;
    for (Thread *colour : *_palette) {
        int distance_sq = pow(needle.R - colour->R, 2) +
                          pow(needle.G - colour->G, 2) +
                          pow(needle.B - colour->B, 2);

        if (distance_sq < minimum_distance_sq) {
            minimum_distance_sq = distance_sq;
            match = colour;
        }
    }

    // Save result in cache
    _nearest_cache[needle] = match;
    return match;
};

// for bayer, you just apply the algorithm you would apply in the 1 bit greyscale case
// to all three channels. Would assume apprx the same approach for floyd steinburg.

RGBcolour image_ptr_to_colour(unsigned char *image_ptr) {
    return RGBcolour{*image_ptr, *(image_ptr+1), *(image_ptr+2)};
}

void apply_quant_error(int err_R, int err_G, int err_B, int *quant_error_ptr, int x, int y, int width, int height, float coefficient) {
    // check bounds
    if (x >= width || y >= height)
        return;

    // apply offset
    quant_error_ptr += INDEX(x, y, width);

    // add error
    *quant_error_ptr += err_R * coefficient;
    *(quant_error_ptr + 1) += err_G * coefficient;
    *(quant_error_ptr + 2) += err_B * coefficient;
}

void FloydSteinburg::dither(unsigned char *image, int width, int height, Project *project) {
    int *quant_error = new int[width * height * 3];
    memset(quant_error, 0, width * height * 3); // initalize quant_error to 0

    int i;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            i = INDEX(x, y, width);
            // It would be ideal to smartly handle opacity. Do not have time
            // to do this. Any areas that are 100% transparent are blank,
            // any other opacity level is treated as 100% opaque.
            if ((int)image[i+3] == 0)
                continue;

            // Fetch pixel, apply any quant error, then find the nearest neighbour.
            RGBcolour old_pixel = image_ptr_to_colour(image + i);
            old_pixel = RGBcolour{old_pixel.R + quant_error[i],
                                  old_pixel.R + quant_error[i+1],
                                  old_pixel.R + quant_error[i+2]};
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
                    project->draw_stitch(nanogui::Vector2i(x, y), new_pixel, j);
                    drawn = true;
                    break;
                }
            }
            if (!drawn) {
                project->palette.push_back(new_pixel);
                project->draw_stitch(nanogui::Vector2i(x, y), new_pixel, project->palette.size() - 1);
            }
        }
    }

    delete[] quant_error;
}

void Bayer::dither(unsigned char *image, int width, int height, Project *project) {
    // TODO
}