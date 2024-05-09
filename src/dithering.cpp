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

std::pair<std::string, std::string> preferred_threads[] = {
    {"DMC", "310"}, {"DMC", "White"}
};

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

auto r_compare = [](const Thread *t1, const Thread *t2) { return t1->R < t2->R; };
auto g_compare = [](const Thread *t1, const Thread *t2) { return t1->G < t2->G; };
auto b_compare = [](const Thread *t1, const Thread *t2) { return t1->B < t2->B; };

std::map<int, Thread*> count_thread_occurances(std::vector<Thread*> *thread_arr, std::vector<Thread*> *prefer_list = nullptr) {
    // count occurances of threads
    std::map<Thread*, int> thread_counts;
    for (Thread *t : *thread_arr) {
        auto iter = thread_counts.find(t);
        if (iter != thread_counts.end()) {
            iter->second++;
        } else {
            thread_counts[t] = 1;
        }
    }

    // If a list of threads to prefer is provided, add 1/3 of the total n.o. pixels to their counts.
    // If one of the threads in the preferred list isn't already in the ideal case dither list it isn't considered.
    if (prefer_list != nullptr) {
        int third_of_total_pixels = thread_arr->size() / 3;
        for (Thread *t : *prefer_list) {
            auto iter = thread_counts.find(t);
            if (iter != thread_counts.end()) {
                iter->second += third_of_total_pixels;
            }
        }
    }

    // reverse map so it can be iterated by count
    std::map<int, Thread*> reversed_thread_counts;
    for (auto const& [key, val] : thread_counts)
        reversed_thread_counts[val] = key;

    return reversed_thread_counts;
}

void DitheringAlgorithm::median_cut(std::vector<Thread*> *image_arr, int depth, std::vector<Thread*> *new_palette) {
    if (image_arr->size() == 0) {
        return;
    }

    // TODO: could instead find the average of these threads, and then find the
    // nearest neighbour to that average within this list. Or within the global list?
    // Pick the locally most popular thread to return. Would need to test that against
    // this approach, and it may even prove to be worth allowing the user to control
    if (depth == 0) {
        std::map<int, Thread*> reversed_thread_counts = count_thread_occurances(image_arr);

        // iterate from high to low key values, add first thread not already in the palette
        for (auto iter = reversed_thread_counts.rbegin(); iter != reversed_thread_counts.rend(); ++iter) {
            bool already_exists = false;
            for (auto t : *new_palette) {
                if (t == iter->second) {
                    already_exists = true;
                    break;
                }
            }

            if (!already_exists) {
                new_palette->push_back(iter->second);
                return;
            }
        }

        // All of the considered threads in this bucket could have already been chosen
        // No point in splitting this bucket further
        return;
    }

    auto r_max = *std::max_element(image_arr->begin(), image_arr->end(), r_compare);
    auto g_max = *std::max_element(image_arr->begin(), image_arr->end(), g_compare);
    auto b_max = *std::max_element(image_arr->begin(), image_arr->end(), b_compare);
    auto r_min = *std::min_element(image_arr->begin(), image_arr->end(), r_compare);
    auto g_min = *std::min_element(image_arr->begin(), image_arr->end(), g_compare);
    auto b_min = *std::min_element(image_arr->begin(), image_arr->end(), b_compare);

    int r_range = r_max->R - r_min->R;
    int g_range = g_max->G - g_min->G;
    int b_range = b_max->B - b_min->B;

    int highest_range = 0;

    if (r_range >= g_range && r_range >= b_range) {
        highest_range = 0;
    } else if (g_range >= r_range && g_range >= b_range) {
        highest_range = 1;
    } else if (b_range >= r_range && b_range >= g_range) {
        highest_range = 2;
    }

    switch (highest_range) {
    case 0: std::sort(image_arr->begin(), image_arr->end(), r_compare);
        break;
    case 1: std::sort(image_arr->begin(), image_arr->end(), g_compare);
        break;
    case 2: std::sort(image_arr->begin(), image_arr->end(), b_compare);
        break;
    }
    int median_index = image_arr->size() / 2;

    auto begin = image_arr->begin();
    auto middle = begin + median_index;
    auto end = image_arr->end();

    std::vector<Thread*> *bucket_1 = new std::vector(begin, middle);
    std::vector<Thread*> *bucket_2 = new std::vector(middle, end);

    median_cut(bucket_1, depth - 1, new_palette);
    median_cut(bucket_2, depth - 1, new_palette);

    delete bucket_1;
    delete bucket_2;
}

void DitheringAlgorithm::reduce_palette(int width, int height, Project *project, std::vector<Thread*> *new_palette) {
    // Median cut can only provide palettes that are powers of 2 so find the largest
    // power of 2 that fits inside _max_threads. All other values of the palette
    // will be picked based on popularity.
    int max_threads_power_2 = std::bit_floor((uint)_max_threads);
    // 2^x = max_threads_power_2, solve for x
    int recursion_depth = floor(log((double)max_threads_power_2) / log(2));

    std::cout << _max_threads << " " << recursion_depth << std::endl;

    // Flatten image into 1 dimension
    auto *flattened_image = new std::vector<Thread*>;
    flattened_image->reserve(width * height);
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int thread_index = project->thread_data[x][y];
            flattened_image->push_back(project->palette[thread_index]);
        }
    }

    // Call median cut algorithm
    median_cut(flattened_image, recursion_depth, new_palette);

    // Fill any remaining palette slots with the remaining most popular threads
    if (new_palette->size() < _max_threads) {
        // Black and White are given priority, as leaving them out when the original
        // dither wanted to use them will very obviously degrade the look of the image.
        std::vector<Thread*> prefer;
        for (auto prefered : preferred_threads) {
            for (Thread *t : *_palette) {
                if (t->number == prefered.second && t->company == prefered.first) {
                    prefer.push_back(t);
                    break;
                }
            }
        }

        std::map<int, Thread*> reversed_thread_counts = count_thread_occurances(flattened_image, &prefer);

        // iterate from high to low key values, add most popular threads not already in palette
        // until _max_threads is reached
        for (auto iter = reversed_thread_counts.rbegin(); iter != reversed_thread_counts.rend(); ++iter) {
            bool already_exists = false;
            for (auto t : *new_palette) {
                if (t == iter->second) {
                    already_exists = true;
                    break;
                }
            }

            if (!already_exists) {
                new_palette->push_back(iter->second);
            }

            if (new_palette->size() == _max_threads) {
                break;
            }
        }
    }

    set_palette(new_palette);

    std::cout << "reduced palette size: " << new_palette->size() << std::endl;

    delete flattened_image;

    // Clear all data written to project
    project->palette.clear();
    for (int i = 0; i < width * height * 4; i++) {
        project->texture_data_array[i] = 255;
    }
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            project->thread_data[x][y] = -1;
        }
    }
}

void DitheringAlgorithm::set_palette(std::vector<Thread*> *new_palette) {
    _nearest_cache.clear();
    _palette = new_palette;
}

void FloydSteinburg::apply_quant_error(int err_R, int err_G, int err_B, int *quant_error_ptr, int x, int y, int width, int height, float coefficient) {
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

void DitheringAlgorithm::draw_stitch(int x, int y, int height, Thread *new_pixel, Project *project) {
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

            draw_stitch(x, y, height, new_pixel, project);
        }
    }

    delete[] quant_error;

    if (project->palette.size() > _max_threads) {
        std::cout << "palette size before reduction: " << project->palette.size() << std::endl;
        std::vector<Thread*> new_palette;
        reduce_palette(width, height, project, &new_palette);
        dither(image, width, height, project);
    }
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
            draw_stitch(x, y, height, new_pixel, project);
        }
    }

    if (project->palette.size() > _max_threads) {
        std::vector<Thread*> new_palette;
        reduce_palette(width, height, project, &new_palette);
        dither(image, width, height, project);
    }
}