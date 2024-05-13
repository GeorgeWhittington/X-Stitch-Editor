#include "dithering.hpp"
#include <set>
#include <nanogui/nanogui.h>
#include <iostream>
#include <numeric>
#include "dkm.hpp"
#include <tuple>

#define FS_ERR_RIGHT      0.4375f // 7/16
#define FS_ERR_DOWN_LEFT  0.1875f // 3/16
#define FS_ERR_DOWN       0.3125f // 5/16
#define FS_ERR_DOWN_RIGHT 0.0625f // 1/16
#define SQ_DIFF(x, y) (((x - y) * (x - y)) >> 2)

RGBcolour BLANK_COLOUR = RGBcolour{};

Thread* DitheringAlgorithm::find_nearest_neighbour(RGBcolour needle, std::vector<Thread*> *palette = nullptr) {
    Thread *match = nullptr;
    bool using_default_palette = palette == nullptr;
    if (using_default_palette) {
        // First check cache
        try {
            match = _nearest_cache.at(needle);
            if (match != nullptr)
                return match;
        } catch (std::out_of_range&) {}

        palette = _palette;
    }

    // Compare needle against all colours in palette to find closest.
    // The channels are very simply weighted according to their perceptual strength
    // in human vision. See: https://en.wikipedia.org/wiki/CIE_1931_color_space
    // The calculation is done with integers for speed, the simplified fractional coefficient is next to each line
    int minimum_distance_sq = INT_MAX;
    for (Thread *colour : *palette) {
        int distance_sq = (1063 * SQ_DIFF(needle.R, colour->R) / 5000) +  // 0.2126
                          (7152 * SQ_DIFF(needle.G, colour->G) / 10000) + // 0.7152
                          (361 * SQ_DIFF(needle.B, colour->B) / 5000);    // 0.0722

        if (distance_sq < minimum_distance_sq) {
            minimum_distance_sq = distance_sq;
            match = colour;
        }
    }

    if (using_default_palette) {
        // Save result in cache
        _nearest_cache[needle] = match;
    }

    return match;
};

auto R_compare = [](const RGBcolour& c1, const RGBcolour& c2) { return c1.R < c2.R; };
auto G_compare = [](const RGBcolour& c1, const RGBcolour& c2) { return c1.G < c2.G; };
auto B_compare = [](const RGBcolour& c1, const RGBcolour& c2) { return c1.B < c2.B; };

void DitheringAlgorithm::median_cut(std::vector<RGBcolour> *image_arr, int depth, std::vector<RGBcolour> *points) {
    if (image_arr->size() == 0) {
        return;
    }

    if (depth == 0) {
        int R_ave = std::accumulate(image_arr->begin(), image_arr->end(), 0.0,
                                       [](int lhs, const RGBcolour& rhs) { return lhs + rhs.R; }) / image_arr->size();
        int G_ave = std::accumulate(image_arr->begin(), image_arr->end(), 0.0,
                                       [](int lhs, const RGBcolour& rhs) { return lhs + rhs.G; }) / image_arr->size();
        int B_ave = std::accumulate(image_arr->begin(), image_arr->end(), 0.0,
                                       [](int lhs, const RGBcolour& rhs) { return lhs + rhs.B; }) / image_arr->size();

        points->push_back({R_ave, G_ave, B_ave});
        return;
    }

    auto R_max = *std::max_element(image_arr->begin(), image_arr->end(), R_compare);
    auto G_max = *std::max_element(image_arr->begin(), image_arr->end(), G_compare);
    auto B_max = *std::max_element(image_arr->begin(), image_arr->end(), B_compare);
    auto R_min = *std::min_element(image_arr->begin(), image_arr->end(), R_compare);
    auto G_min = *std::min_element(image_arr->begin(), image_arr->end(), G_compare);
    auto B_min = *std::min_element(image_arr->begin(), image_arr->end(), B_compare);

    double R_range = R_max.R - R_min.R;
    double G_range = G_max.G - G_min.G;
    double B_range = B_max.B - B_min.B;

    int highest_range = 0;

    if (R_range >= G_range && R_range >= B_range) {
        highest_range = 0;
    } else if (G_range >= R_range && G_range >= B_range) {
        highest_range = 1;
    } else if (B_range >= R_range && B_range >= G_range) {
        highest_range = 2;
    }

    switch (highest_range) {
    case 0: std::sort(image_arr->begin(), image_arr->end(), R_compare);
        break;
    case 1: std::sort(image_arr->begin(), image_arr->end(), G_compare);
        break;
    case 2: std::sort(image_arr->begin(), image_arr->end(), B_compare);
        break;
    }
    int median_index = image_arr->size() / 2;

    auto begin = image_arr->begin();
    auto middle = begin + median_index;
    auto end = image_arr->end();

    std::vector<RGBcolour> *bucket_1 = new std::vector(begin, middle);
    std::vector<RGBcolour> *bucket_2 = new std::vector(middle, end);

    median_cut(bucket_1, depth - 1, points);
    median_cut(bucket_2, depth - 1, points);

    delete bucket_1;
    delete bucket_2;
}

void DitheringAlgorithm::create_closest_palette(std::vector<RGBcolour> averages, std::vector<Thread*> *new_palette) {
    for (auto average : averages) {
        std::vector<Thread*> palette = *_palette;

        while (true) {
            if (palette.size() == 0)
                break;

            Thread *t = find_nearest_neighbour(average, &palette);
            bool already_in_palette = false;
            for (Thread *pt : *new_palette) {
                if (t == pt) {
                    already_in_palette = true;
                    break;
                }
            }

            if (!already_in_palette) {
                new_palette->push_back(t);
                break;
            }

            for (int i = 0; i < palette.size(); i++) {
                if (t == palette[i]) {
                    palette.erase(palette.begin() + i);
                    break;
                }
            }
        }
    }

    std::cout << "palette reduced to: " << new_palette->size() << std::endl;
    set_palette(new_palette);
}

void DitheringAlgorithm::reduce_palette(unsigned char *image, int width, int height, std::vector<Thread*> *new_palette, bool median_cut_floor) {
    auto *image_vector = new std::vector<RGBcolour>;
    std::vector<RGBcolour> median_cut_points;
    for (int i = 0; i < width * height; i++) {
        // Ignore 100% transparent pixels
        if (image[(i*4)+3] == 0)
            continue;
        image_vector->push_back({image[i*4], image[(i*4)+1], image[(i*4)+2]});
    }

    // Median cut can only provide palettes that are powers of 2 so find the smallest
    // power of 2 that is larger than _max_threads. The results will be clustered to
    // be exactly _max_threads using k-means.
    int max_threads_power_2 = std::bit_ceil((uint)_max_threads);
    // 2^x = max_threads_power_2, solve for x
    int recursion_depth = floor(log((double)max_threads_power_2) / log(2));

    median_cut(image_vector, recursion_depth, &median_cut_points);

    // palette is already less than max_threads, set palette and return
    if (median_cut_points.size() <= _max_threads) {
        create_closest_palette(median_cut_points, new_palette);
        delete image_vector;
        return;
    }

    std::vector<std::array<double, 3>> kmeans_data;
    for (auto mean : median_cut_points) {
        kmeans_data.push_back({(double)mean.R, (double)mean.G, (double)mean.B});
    }

    // Reduce palette further with k-means
    dkm::clustering_parameters<double> params = dkm::clustering_parameters<double>(_max_threads);
    params.set_min_delta(0.0005);
    const auto [means, mapping] = dkm::kmeans_lloyd(kmeans_data, params);

    std::vector<RGBcolour> points;
    for (auto mean : means) {
        points.push_back(RGBcolour{(int)mean[0], (int)mean[1], (int)mean[2]});
    }

    create_closest_palette(points, new_palette);
    delete image_vector;
    return;
}

struct Lab {float L; float a; float b;};

// from: https://bottosson.github.io/posts/oklab/
Lab thread_to_oklab(Thread *t) {
    float l = 0.4122214708f * t->R + 0.5363325363f * t->G + 0.0514459929f * t->B;
	float m = 0.2119034982f * t->R + 0.6806995451f * t->G + 0.1073969566f * t->B;
	float s = 0.0883024619f * t->R + 0.2817188376f * t->G + 0.6299787005f * t->B;

    float l_ = cbrtf(l);
    float m_ = cbrtf(m);
    float s_ = cbrtf(s);

    return {
        0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_,
        1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_,
        0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_,
    };
}

double oklab_total_distance(Lab lab1, Lab lab2) {
    // using distance formula from: https://github.com/color-js/color.js/blob/d49b1ae0a571f700dd09aa777da595d681b1d17b/src/deltaE/deltaEOK2.js
    double delta_L = lab1.L - lab2.L;
    double delta_a = 2.0 * (lab1.a - lab2.a);
    double delta_b = 2.0 * (lab1.b - lab2.b);
    return sqrt((delta_L * delta_L) + (delta_a * delta_a) + (delta_b * delta_b));
}

double oklab_hue_distance(Lab lab1, Lab lab2) {
    // using hue distance from: https://github.com/svgeesus/svgeesus.github.io/blob/master/Color/OKLab-notes.md#color-difference-metric
    double delta_L = lab1.L - lab2.L;
    double delta_C = sqrt(pow(lab1.a, 2) + pow(lab1.b, 2)) - sqrt(pow(lab2.a, 2) + pow(lab2.b, 2));
    double delta_a = lab1.a - lab2.a;
    double delta_b = lab1.b - lab2.b;
    return sqrt(pow(delta_a, 2) + pow(delta_b, 2) - pow(delta_C, 2));
}

void DitheringAlgorithm::expand_palette(std::vector<Thread*> *new_palette) {
    std::vector<BlendedThread*> added_threads;

    for (Thread *t1 : *_palette) {
        new_palette->push_back(t1);

        for (Thread *t2 : *_palette) {
            // Don't blend threads with themselves
            if (t1 == t2)
                continue;

            // Only consider blending threads which are close in colour
            // Testing difference in Oklab colourspace, since perceptual uniformity
            // is more important here
            Lab lab1 = thread_to_oklab(t1);
            Lab lab2 = thread_to_oklab(t2);
            double distance = oklab_total_distance(lab1, lab2);
            double ave_L = (lab1.L + lab2.L) / 2.0;
            // adding to distance using a `y = 0.7 sqrt(x - 5.4)` curve, as colours that are pastel
            // (ie: have high L values, above 5.5 but usually closer to 6.0) will otherwise slip past
            // the threshold.
            // see: https://graphicdesign.stackexchange.com/questions/164653/color-difference-functions-are-not-good-what-am-i-missing
            distance = ave_L >= 5.4 ? distance + (0.7 * sqrt(ave_L - 5.4)) : distance;
            if (distance > 1.0)
                continue;

            // Colours that have similar lightnesses can still clash if their hue is very different
            double hue_distance = oklab_hue_distance(lab1, lab2);
            if (hue_distance > 0.3)
                continue;

            // Ignore any duplicate combinations
            BlendedThread *bt1 = new BlendedThread(create_blended_thread((SingleThread*)t1, (SingleThread*)t2));
            bool duplicate = false;
            for (BlendedThread *bt2 : added_threads) {
                if (is_duplicate(bt1, bt2)) {
                    delete bt1;
                    duplicate = true;
                    break;
                }
            }
            if (duplicate)
                continue;

            // std::cout << fmt::format("{} -> {}, {}, {} | {}, {}, {} -> {}, {} -> L:{} a:{} b:{} | L:{} a:{} b:{}", bt1->full_name(ThreadPosition::BOTH), t1->R, t1->G, t1->B, t2->R, t2->G, t2->B, distance, hue_distance, lab1.L, lab1.a, lab1.b, lab2.L, lab2.a, lab2.b) << std::endl;

            added_threads.push_back(bt1);
            new_palette->push_back(bt1);
        }
    }

    set_palette(new_palette);
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
    std::vector<Thread*> new_palette;
    if (_palette->size() > _max_threads) {
        reduce_palette(image, width, height, &new_palette);
    }

    std::vector<Thread*> new_new_palette;
    if (_blend_threads) {
        expand_palette(&new_new_palette);
    }

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
}

void NoDither::dither(unsigned char *image, int width, int height, Project *project) {
    std::vector<Thread*> new_palette;
    if (_palette->size() > _max_threads) {
        reduce_palette(image, width, height, &new_palette);
    }

    std::vector<Thread*> new_new_palette;
    if (_blend_threads) {
        expand_palette(&new_new_palette);
    }

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
}