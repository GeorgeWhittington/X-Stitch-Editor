#include <iostream>
#include <map>
#include <nanogui/nanogui.h>
#include <freetype/ftstroke.h>
#include "pdf_creation.hpp"
#include "threads.hpp"
#include <fmt/core.h>
#include <ctime>
#include <limits>
#include "constants.hpp"

using nanogui::Vector2f;

const float A4_WIDTH = 595.f;
const float A4_HEIGHT = 842.f;
const float PAGE_MARGIN = 60.f;

const float CHART_WIDTH = A4_WIDTH - 95.f; // 500 -> 50 stitches
const float CHART_HEIGHT = A4_HEIGHT - 102.f; // 740 -> 74 stitches
const float CHART_X = 47.5f;
const float CHART_Y = 51.f;
const float CHART_STITCH_WIDTH = 10.f;
const int CHART_STITCHES_X = CHART_WIDTH / CHART_STITCH_WIDTH;
const int CHART_STITCHES_Y = CHART_HEIGHT / CHART_STITCH_WIDTH;

const std::string symbol_key_headers[4] = {"Symbol", "Number", "Stitches", "Backstitches"};

const int MAX_SYMBOLS = 84;
const std::string symbols[MAX_SYMBOLS] = {
    "air", "aircraft", "arrow-down", "arrow-left", "arrow-up",
    "arrow-right", "arrow-with-circle-down", "arrow-with-circle-left",
    "arrow-with-circle-right", "arrow-with-circle-up", "attachment",
    "beamed-note", "bell", "book", "bug", "check",
    "chevron-down", "chevron-left", "chevron-right", "chevron-up",
    "circle-with-cross", "circle-with-minus", "circle-with-plus",
    "circle", "cloud", "controller-fast-backward", "controller-fast-forward",
    "controller-jump-to-start", "controller-next", "controller-pause",
    "controller-play", "controller-record", "controller-stop", "credit",
    "dots-two-horizontal", "dots-two-vertical", "drop", "eraser", "feather",
    "flag", "flash", "flower", "hand", "heart", "infinity", "key", "lab-flask",
    "leaf", "lifebuoy", "light-bulb", "lock-open", "lock", "magnet",
    "magnifying-glass", "mail", "minus", "modern-mic", "moon", "note", "open-book",
    "palette", "pencil", "phone", "rainbow", "rocket", "select-arrows",
    "squared-cross", "squared-minus", "squared-plus", "star-outlined",
    "star", "traffic-cone", "trash", "tree", "triangle-down", "triangle-left",
    "triangle-right", "triangle-up", "trophy", "vinyl", "voicemail", "wallet",
    "warning", "water"
};

// TODO: hard limit of 7 dash patterns atm, should come up with more, or
// make it so that they wrap around and start getting drawn in greyscale (0.75, 0.5, 0.25)
// so that there's 28 possible options
const DashPattern dash_patterns[] = {
    {},
    {5.0, 5.0},
    {5.0, 10.0},
    {10.0, 5.0},
    {2.0, 3.0},
    {2.0, 5.0, 4.0, 5.0},
    {1.0, 5.0, 6.0, 5.0}
};

void draw_line(PageContentContext *ctx, float x1, float y1, float x2, float y2) {
    ctx->m(x1, y1); // move
    ctx->l(x2, y2); // draw line to
    ctx->S(); // stroke
}

PDFWizard::PDFWizard(Project *project, PDFSettings *settings) : _project(project), _settings(settings) {};

void PDFWizard::create_and_save_pdf(std::string path) {
    if (_pdf_writer.StartPDF(path, ePDFVersion13) != eSuccess)
        throw std::runtime_error("Error creating PDFWriter");

    std::string resources_dir = get_resources_dir();
    if (resources_dir == "")
        throw std::runtime_error("Couldn't fetch resource directory");

    _helvetica = _pdf_writer.GetFontForFile(resources_dir + "/Helvetica.ttf");
    if (!_helvetica)
        throw std::runtime_error("Error loading font Helvetica for PDF generation");
    _helvetica_bold = _pdf_writer.GetFontForFile(resources_dir + "/Helvetica-Bold.ttf");
    if (!_helvetica_bold)
        throw std::runtime_error("Error loading font Helvetica-Bold for PDF generation");

    auto title_text_options = AbstractContentContext::TextOptions(_helvetica, 24, AbstractContentContext::eGray, 0);
    auto body_text_options = AbstractContentContext::TextOptions(_helvetica, 14, AbstractContentContext::eGray, 0);
    auto body_bold_text_options = AbstractContentContext::TextOptions(_helvetica_bold, 14, AbstractContentContext::eGray, 0);
    auto body_small_text_options = AbstractContentContext::TextOptions(_helvetica, 10, AbstractContentContext::eGray, 0);
    _title_text_options = &title_text_options;
    _body_text_options = &body_text_options;
    _body_bold_text_options = &body_bold_text_options;
    _body_small_text_options = &body_small_text_options;

    fetch_symbol_data();

    int backstitch_multiplier = _settings->render_backstitch_chart ? 2 : 1;
    _pattern_no_pages_wide = std::ceil((float)_project->width / (float)CHART_STITCHES_X);
    _pattern_no_pages_tall = std::ceil((float)_project->height / (float)CHART_STITCHES_Y);
    _total_pages = 2 + ((_pattern_no_pages_wide * _pattern_no_pages_tall) * backstitch_multiplier);

    create_title_page();
    create_symbol_key_page();
    create_chart_pages(false);
    if (_settings->render_backstitch_chart)
        create_chart_pages(true);

    if (_pdf_writer.EndPDF() != eSuccess)
        throw std::runtime_error("Error writing PDF file");
}

void PDFWizard::create_title_page() {
    PDFPage *page = new PDFPage();
    page->SetMediaBox(PDFRectangle(0, 0, A4_WIDTH, A4_HEIGHT));
    PageContentContext *page_content_ctx = _pdf_writer.StartPageContentContext(page);

    std::string title = _project->title + " by " + _settings->author;
    page_content_ctx->WriteText(PAGE_MARGIN, A4_HEIGHT - PAGE_MARGIN - 10, title, *_title_text_options);

    // Calculate pattern preview scale
    float page_width = A4_WIDTH - (PAGE_MARGIN * 2);
    float page_height = A4_HEIGHT - (PAGE_MARGIN * 2) - 40;
    float page_x = PAGE_MARGIN;
    float page_y = PAGE_MARGIN;
    float stitch_width;

    // Scale by different axis depending on which aspect ratio is larger
    if ((page_height / page_width) >= ((float)_project->height / (float)_project->width)) {
        stitch_width = page_width / _project->width;
        float new_page_height = stitch_width * _project->height;
        float vertical_margin = (page_height - new_page_height) / 2;
        page_height = new_page_height;
        page_y += vertical_margin;
    } else {
        stitch_width = page_height / _project->height;
        float new_page_width = stitch_width * _project->width;
        float horizontal_margin = (page_width - new_page_width) / 2;
        page_width = new_page_width;
        page_x += horizontal_margin;
    }

    page_content_ctx->w(1.f); // line width
    page_content_ctx->J(FT_Stroker_LineCap_::FT_STROKER_LINECAP_BUTT); // set end cap

    int stitch;
    nanogui::Color c;
    for (int x = 0; x < _project->width; x++) {
        for (int y = 0; y < _project->height; y++) {
            stitch = _project->thread_data[x][y];
            if (stitch == -1) {
                c = nanogui::Color(nanogui::Vector3f(1.f, 1.f, 1.f));
            } else {
                c = _project->palette[stitch]->color();
            }

            float x_start = page_x + (x * stitch_width);
            float x_end = x_start + stitch_width;
            float y_start = page_y + (y * stitch_width);
            float y_end = y_start + stitch_width;

            page_content_ctx->rg(c.r(), c.g(), c.b()); // set fill colour
            page_content_ctx->RG(c.r(), c.g(), c.b()); // set stroke colour

            // Draw filled rectangle
            page_content_ctx->re(x_start, y_start, stitch_width, stitch_width); // draw rectangle
            page_content_ctx->f(); // fill

            // Draw line from tl to tr
            draw_line(page_content_ctx, x_start, y_end, x_end, y_end);

            // Draw line from tr to br
            draw_line(page_content_ctx, x_end, y_end, x_end, y_start);
        }
    }

    page_content_ctx->w(2.f); // line width
    page_content_ctx->J(FT_Stroker_LineCap_::FT_STROKER_LINECAP_ROUND); // set end cap

    for (BackStitch bs : _project->backstitches) {
        c = _project->palette[bs.palette_index]->color();
        page_content_ctx->RG(c.r(), c.g(), c.b()); // set stroke colour
        draw_line(page_content_ctx, page_x + (bs.start[0] * stitch_width), page_y + (bs.start[1] * stitch_width),
                                    page_x + (bs.end[0] * stitch_width), page_y + (bs.end[1] * stitch_width));
    }

    // Stroke pattern outline
    page_content_ctx->w(2.0); // line width
    page_content_ctx->RG(0.f, 0.f, 0.f); // set stroke colour

    page_content_ctx->re(page_x, page_y, page_width, page_height); // draw rectangle
    page_content_ctx->S(); // stroke

    save_page(page, page_content_ctx, PAGE_MARGIN);
}

void PDFWizard::create_symbol_key_page() {
    PDFPage *page = new PDFPage();
    page->SetMediaBox(PDFRectangle(0, 0, A4_WIDTH, A4_HEIGHT));
    PageContentContext *page_content_ctx = _pdf_writer.StartPageContentContext(page);

    // Find longest item per column, to calculate table spacing
    float longest_in_column[3];
    for (int i = 0; i < 3; i++) {
        auto text_dimensions = _helvetica_bold->CalculateTextDimensions(symbol_key_headers[i], 14);
        longest_in_column[i] = text_dimensions.width;
    }
    for (TableRow *row : _symbol_key_rows) {
        auto num_text_dimensions = _helvetica->CalculateTextDimensions(row->number, 14);
        if (num_text_dimensions.width > longest_in_column[1])
            longest_in_column[1] = num_text_dimensions.width;

        auto no_stitches_text_dimensions = _helvetica->CalculateTextDimensions(std::to_string(row->no_stitches), 14);
        if (no_stitches_text_dimensions.width > longest_in_column[2])
            longest_in_column[2] = no_stitches_text_dimensions.width;
    }

    page_content_ctx->WriteText(PAGE_MARGIN, A4_HEIGHT - PAGE_MARGIN - 10, "Symbol Key", *_title_text_options);

    float x = PAGE_MARGIN;
    float y = A4_HEIGHT - 100;
    float column_spacing = 20.f;

    for (int i = 0; i < 4; i++) {
        page_content_ctx->WriteText(x, y, symbol_key_headers[i], *_body_bold_text_options);
        if (i != 3)
            x += longest_in_column[i] + column_spacing;
    }

    y -= 5;

    // Draw horizontal line under headers
    page_content_ctx->w(1.f); // line width
    page_content_ctx->J(FT_Stroker_LineCap_::FT_STROKER_LINECAP_BUTT); // set end cap
    page_content_ctx->RG(0.f, 0.f, 0.f); // set stroke colour
    draw_line(page_content_ctx, PAGE_MARGIN, y, A4_WIDTH - PAGE_MARGIN, y);

    x = PAGE_MARGIN;
    y -= 15;

    AbstractContentContext::ImageOptions image_options;
    image_options.transformationMethod = AbstractContentContext::eFit;
    image_options.boundingBoxWidth = 14.f;
    image_options.boundingBoxHeight = 14.f;
    page_content_ctx->J(FT_Stroker_LineCap_::FT_STROKER_LINECAP_ROUND); // set end cap
    nanogui::Color c;

    for (TableRow *row : _symbol_key_rows) {
        if (y < PAGE_MARGIN) {
            // Page is full, save and create a new one to continue on
            save_page(page, page_content_ctx, PAGE_MARGIN);

            page = new PDFPage();
            page->SetMediaBox(PDFRectangle(0, 0, A4_WIDTH, A4_HEIGHT));
            page_content_ctx = _pdf_writer.StartPageContentContext(page);
            y = A4_HEIGHT - PAGE_MARGIN - 10;

            // Redraw headers
            for (int i = 0; i < 4; i++) {
                page_content_ctx->WriteText(x, y, symbol_key_headers[i], *_body_bold_text_options);
                if (i != 3)
                    x += longest_in_column[i] + column_spacing;
            }

            y -= 5;

            // Draw horizontal line under headers
            page_content_ctx->w(1.f); // line width
            page_content_ctx->J(FT_Stroker_LineCap_::FT_STROKER_LINECAP_BUTT); // set end cap
            page_content_ctx->RG(0.f, 0.f, 0.f); // set stroke colour
            draw_line(page_content_ctx, PAGE_MARGIN, y, A4_WIDTH - PAGE_MARGIN, y);

            x = PAGE_MARGIN;
            y -= 15;
        }

        c = _project->palette[row->palette_id]->color();
        if (row->no_stitches != 0) {
            Thread *t = _project->palette[row->palette_id];

            if (!t->is_blended()) {
                page_content_ctx->rg(c.r(), c.g(), c.b()); // set fill colour
                page_content_ctx->re(x, y - 6, 18, 18); // draw rectangle
                page_content_ctx->f(); // fill
            } else {
                BlendedThread *bt = (BlendedThread*)t;
                nanogui::Color c1 = bt->thread_1->color();
                nanogui::Color c2 = bt->thread_2->color();

                // Drawing two triangles that make up a square
                page_content_ctx->rg(c1.r(), c1.g(), c1.b()); // set fill colour
                page_content_ctx->m(x, y - 6); // move cursor
                page_content_ctx->l(x + 18, y - 6); // draw line to
                page_content_ctx->l(x, y - 6 + 18); // draw line to
                page_content_ctx->l(x, y - 6); // draw line to
                page_content_ctx->f(); // fill

                page_content_ctx->rg(c2.r(), c2.g(), c2.b()); // set fill colour
                page_content_ctx->m(x + 18, y - 6 + 18); // move cursor
                page_content_ctx->l(x + 18, y - 6); // draw line to
                page_content_ctx->l(x, y - 6 + 18); // draw line to
                page_content_ctx->l(x + 18, y - 6 + 18); // draw line to
                page_content_ctx->f(); // fill
            }

            if (_settings->render_in_colour) {
                page_content_ctx->DrawImage(x + 2, y - 4, _project_symbols[row->palette_id], image_options);
            } else {
                page_content_ctx->w(0.5); // set line width
                page_content_ctx->RG(0, 0, 0); // set stroke colour
                page_content_ctx->re(x + 20, y - 6, 18, 18); // draw rectangle
                page_content_ctx->S(); // stroke

                page_content_ctx->DrawImage(x + 22, y - 4, _project_symbols[row->palette_id], image_options);
            }
        }
        if (row->no_backstitches != 0) {
            page_content_ctx->w(2.f); // set line width
            if (_settings->render_in_colour) {
                page_content_ctx->RG(c.r(), c.g(), c.b()); // set stroke colour
                draw_line(page_content_ctx, x + 22, y - 4, x + 44, y + 10);
            } else {
                page_content_ctx->q();
                DashPattern dash_pattern = _project_dashpatterns[row->palette_id];
                if (dash_pattern.size() != 0) {
                    page_content_ctx->d(dash_pattern.data(), dash_pattern.size(), 0);
                }
                draw_line(page_content_ctx, x + 42, y - 4, x + 64, y + 10);
                page_content_ctx->Q();
            }
        }
        x += longest_in_column[0] + column_spacing;

        page_content_ctx->WriteText(x, y, row->number, *_body_text_options);
        x += longest_in_column[1] + column_spacing;

        page_content_ctx->WriteText(x, y, std::to_string(row->no_stitches), *_body_text_options);
        x += longest_in_column[2] + column_spacing;

        page_content_ctx->WriteText(x, y, std::to_string(row->no_backstitches), *_body_text_options);

        x = PAGE_MARGIN;
        y -= 20;
    }

    save_page(page, page_content_ctx, PAGE_MARGIN);
}

void PDFWizard::draw_gridlines(PageContentContext *ctx, PatternPageContext p_ctx) {
    // Draw minor grid lines
    ctx->J(FT_Stroker_LineCap_::FT_STROKER_LINECAP_SQUARE); // set end cap
    ctx->w(0.35f); // line width
    ctx->RG(0.6f, 0.6f, 0.6f); // set stroke colour
    for (int x = p_ctx.x_start; x < p_ctx.x_end; x++) {
        int rel_x = p_ctx.x_start != 0 ? x - p_ctx.x_start : x;
        float pos_x = p_ctx.chart_x + (rel_x * p_ctx.stitch_width);

        // Draw minor vertical grid lines
        if (x > p_ctx.x_start)
            draw_line(ctx, pos_x, p_ctx.chart_y, pos_x, p_ctx.chart_y_end);

        for (int y = p_ctx.y_start; y < p_ctx.y_end; y++) {
            int rel_y = p_ctx.y_start != 0 ? y - p_ctx.y_start : y;
            float pos_y = p_ctx.chart_y + (rel_y * p_ctx.stitch_width);

            // Draw minor horizontal grid lines
            if (x == p_ctx.x_start && y > p_ctx.y_start)
                draw_line(ctx, p_ctx.chart_x, pos_y, p_ctx.chart_x_end, pos_y);
        }
    }

    // Draw major gridlines and line no's
    ctx->w(1.f); // line width
    ctx->RG(0.f, 0.f, 0.f); // set stroke colour
    for (int x = p_ctx.x_start; x <= p_ctx.x_end; x++) {
        if (x % 10 != 0)
            continue;

        int rel_x = p_ctx.x_start != 0 ? x - p_ctx.x_start : x;
        float pos_x = p_ctx.chart_x + (rel_x * p_ctx.stitch_width);
        draw_line(ctx, pos_x, p_ctx.chart_y, pos_x, p_ctx.chart_y_end);

        // Write line no
        std::string x_str = std::to_string(x);
        float start = pos_x - 20.f;
        auto x_text_dimensions = _helvetica->CalculateTextDimensions(x_str, 10);
        float offset = (40.f - x_text_dimensions.width) / 2.f;
        ctx->WriteText(start + offset, p_ctx.chart_y_end + 5.f, x_str, *_body_small_text_options);

        for (int y = p_ctx.y_start; y <= p_ctx.y_end; y++) {
            int y_inverted = _project->height - y;
            if (y_inverted % 10 != 0 || y_inverted == 0)
                continue;

            int rel_y = p_ctx.y_start != 0 ? y - p_ctx.y_start : y;
            float pos_y = p_ctx.chart_y + (rel_y * p_ctx.stitch_width);
            draw_line(ctx, p_ctx.chart_x, pos_y, p_ctx.chart_x_end, pos_y);

            // Write line no
            std::string y_str = std::to_string(y_inverted);
            float start = pos_y - 20.f;
            auto y_text_dimensions = _helvetica->CalculateTextDimensions(y_str, 10);
            float offset = (40.f - y_text_dimensions.height) / 2.f;
            ctx->WriteText(p_ctx.chart_x - y_text_dimensions.width - 5.f, start + offset, y_str, *_body_small_text_options);
        }
    }

    // Draw Outline
    ctx->w(1.5f); // line width
    ctx->RG(0.f, 0.f, 0.f); // set stroke colour
    ctx->re(p_ctx.chart_x, p_ctx.chart_y,
            (p_ctx.x_end - p_ctx.x_start) * p_ctx.stitch_width,
            (p_ctx.y_end - p_ctx.y_start) * p_ctx.stitch_width); // draw rectangle
    ctx->S(); // stroke
}

bool backstitch_inside_chart(BackStitch bs, float x_start, float x_end, float y_start, float y_end) {
    int inside = 0;
    for (Vector2f point : {bs.start, bs.end})
        if (point[0] >= x_start && point[0] <= x_end && point[1] >= y_start && point[1] <= y_end)
            inside++;

    return inside == 2;
}

bool backstitch_intersects(BackStitch bs, float x_start, float x_end, float y_start, float y_end) {
    for (Vector2f point : {bs.start, bs.end})
        if (point[0] >= x_start && point[0] <= x_end && point[1] >= y_start && point[1] <= y_end)
            return true;

    Vector2f bl = Vector2f(x_start, y_start);
    Vector2f br = Vector2f(x_end, y_start);
    Vector2f tl = Vector2f(x_start, y_end);
    Vector2f tr = Vector2f(x_end, y_end);

    std::pair<Vector2f, Vector2f> edges[4] = {
        std::pair(tl, tr), // top
        std::pair(bl, br), // bottom
        std::pair(bl, tl), // left
        std::pair(br, tr), // right
    };

    for (std::pair<Vector2f, Vector2f> edge : edges) {
        if (test_intersection(bs.start, bs.end, edge.first, edge.second)) {
            return true;
        }
    }

    return false;
}

void PDFWizard::draw_backstitches(PageContentContext *ctx, PatternPageContext p_ctx, bool over_symbols) {
    ctx->w(2.f); // set line width
    ctx->J(FT_Stroker_LineCap_::FT_STROKER_LINECAP_ROUND); // set end cap
    if (_settings->render_in_colour) {
        ctx->RG(1.f, 0.f, 0.f); // set fill colour
    } else {
        ctx->RG(0.f, 0.f, 0.f); // set fill colour
    }

    for (BackStitch bs : _project->backstitches) {
        if (!backstitch_intersects(bs, p_ctx.x_start, p_ctx.x_end, p_ctx.y_start, p_ctx.y_end))
            continue;

        float rel_x1 = p_ctx.x_start != 0 ? bs.start[0] - p_ctx.x_start : bs.start[0];
        float rel_x2 = p_ctx.x_start != 0 ? bs.end[0] - p_ctx.x_start : bs.end[0];
        float rel_y1 = p_ctx.y_start != 0 ? bs.start[1] - p_ctx.y_start : bs.start[1];
        float rel_y2 = p_ctx.y_start != 0 ? bs.end[1] - p_ctx.y_start : bs.end[1];
        float pos_x1 = p_ctx.chart_x + (rel_x1 * p_ctx.stitch_width);
        float pos_x2 = p_ctx.chart_x + (rel_x2 * p_ctx.stitch_width);
        float pos_y1 = p_ctx.chart_y + (rel_y1 * p_ctx.stitch_width);
        float pos_y2 = p_ctx.chart_y + (rel_y2 * p_ctx.stitch_width);

        Vector2f bl(p_ctx.chart_x, p_ctx.chart_y);
        Vector2f br(p_ctx.chart_x_end, p_ctx.chart_y);
        Vector2f tl(p_ctx.chart_x, p_ctx.chart_y_end);
        Vector2f tr(p_ctx.chart_x_end, p_ctx.chart_y_end);

        std::pair<Vector2f, Vector2f> chart_edges[4] = {
            std::pair(tl, tr), // top
            std::pair(bl, br), // bottom
            std::pair(bl, tl), // left
            std::pair(br, tr)  // right
        };

        if ((_settings->render_in_colour && !over_symbols) ||
            (_settings->render_in_colour && over_symbols && !_settings->render_backstitch_chart)) {
            nanogui::Color c = _project->palette[bs.palette_index]->color();
            ctx->RG(c.r(), c.g(), c.b()); // set stroke colour
        }

        if (backstitch_inside_chart(bs, p_ctx.x_start, p_ctx.x_end, p_ctx.y_start, p_ctx.y_end)) {
            // backstitch fully inside chart, just draw
            DashPattern dash_pattern = _project_dashpatterns[bs.palette_index];
            ctx->q();
            if (dash_pattern.size() != 0 && _settings->render_in_colour == false)
                ctx->d(dash_pattern.data(), dash_pattern.size(), 0);
            draw_line(ctx, pos_x1, pos_y1, pos_x2, pos_y2);
            ctx->Q();
        } else {
            // backstitch has some point(s) outside chart, find where it intersects with the edge
            std::vector<Vector2f> intersections;

            // is one backstitch edge inside the chart?
            if (bs.start[0] >= p_ctx.x_start && bs.start[0] <= p_ctx.x_end && bs.start[1] >= p_ctx.y_start && bs.start[1] <= p_ctx.y_end) {
                intersections.push_back(Vector2f(pos_x1, pos_y1));
            } else if (bs.end[0] >= p_ctx.x_start && bs.end[0] <= p_ctx.x_end && bs.end[1] >= p_ctx.y_start && bs.end[1] <= p_ctx.y_end) {
                intersections.push_back(Vector2f(pos_x2, pos_y2));
            }

            for (int i = 0; i < 4; i++) {
                std::pair<Vector2f, Vector2f> edge = chart_edges[i];
                Vector2f *intersection = get_intersection(Vector2f(pos_x1, pos_y1), Vector2f(pos_x2, pos_y2), edge.first, edge.second);

                if (intersection != nullptr) {
                    bool collision = false;
                    for (Vector2f i : intersections) {
                        if (i == *intersection) {
                            collision = true;
                            break;
                        }
                    }

                    if (collision == false)
                        intersections.push_back(*intersection);
                    delete intersection;
                }
            }

            if (intersections.size() == 2) {
                DashPattern dash_pattern = _project_dashpatterns[bs.palette_index];
                ctx->q();
                if (dash_pattern.size() != 0 && _settings->render_in_colour == false)
                    ctx->d(dash_pattern.data(), dash_pattern.size(), 0);
                draw_line(ctx, intersections[0][0], intersections[0][1], intersections[1][0], intersections[1][1]);
                ctx->Q();
            }
        }
    }
}

void PDFWizard::draw_chart(PageContentContext *ctx, PatternPageContext p_ctx, bool backstitch_only) {
    if (!backstitch_only) {
        // Draw symbols and optionally colour
        AbstractContentContext::ImageOptions image_options;
        image_options.transformationMethod = AbstractContentContext::eFit;
        image_options.boundingBoxWidth = p_ctx.stitch_width - 3.f;
        image_options.boundingBoxHeight = p_ctx.stitch_width - 3.f;
        for (int x = p_ctx.x_start; x < p_ctx.x_end; x++) {
            int rel_x = p_ctx.x_start != 0 ? x - p_ctx.x_start : x;
            float pos_x = p_ctx.chart_x + (rel_x * p_ctx.stitch_width);

            for (int y = p_ctx.y_start; y < p_ctx.y_end; y++) {
                int rel_y = p_ctx.y_start != 0 ? y - p_ctx.y_start : y;
                float pos_y = p_ctx.chart_y + (rel_y * p_ctx.stitch_width);

                int palette_index = _project->thread_data[x][y];
                if (palette_index == -1)
                    continue;

                if (_settings->render_in_colour) {
                    nanogui::Color c = _project->palette[palette_index]->color();
                    ctx->rg(c.r(), c.g(), c.b()); // set fill colour
                    ctx->re(pos_x, pos_y, p_ctx.stitch_width, p_ctx.stitch_width); // draw rectangle
                    ctx->f(); // fill
                }

                ctx->DrawImage(pos_x + 1.5f, pos_y + 1.5f, _project_symbols[palette_index], image_options);
            }
        }
    }

    draw_gridlines(ctx, p_ctx);
    if (_project->backstitches.size() != 0)
        draw_backstitches(ctx, p_ctx, !backstitch_only);
}

void PDFWizard::create_chart_pages(bool backstitch_only) {
    if (_pattern_no_pages_wide == 1 && _pattern_no_pages_tall == 1) {
        // special case, scale to fit max width/height (similar to preview)
        PDFPage *page = new PDFPage();
        page->SetMediaBox(PDFRectangle(0, 0, A4_WIDTH, A4_HEIGHT));
        PageContentContext *page_content_ctx = _pdf_writer.StartPageContentContext(page);

        float chart_width = CHART_WIDTH;
        float chart_height = CHART_HEIGHT;

        PatternPageContext p_ctx;
        p_ctx.chart_x = CHART_X;
        p_ctx.chart_y = CHART_Y;
        p_ctx.x_start = 0;
        p_ctx.x_end = _project->width;
        p_ctx.y_start = 0;
        p_ctx.y_end = _project->height;

        // Scale by different axis depending on which aspect ratio is larger
        if ((chart_height / chart_width) >= ((float)_project->height / (float)_project->width)) {
            p_ctx.stitch_width = chart_width / _project->width;
            float new_page_height = p_ctx.stitch_width * _project->height;
            float vertical_margin = (chart_height - new_page_height) / 2;
            chart_height = new_page_height;
            p_ctx.chart_y += vertical_margin;
        } else {
            p_ctx.stitch_width = chart_height / _project->height;
            float new_page_width = p_ctx.stitch_width * _project->width;
            float horizontal_margin = (chart_width - new_page_width) / 2;
            chart_width = new_page_width;
            p_ctx.chart_x += horizontal_margin;
        }

        p_ctx.chart_x_end = p_ctx.chart_x + (_project->width * p_ctx.stitch_width);
        p_ctx.chart_y_end = p_ctx.chart_y + (_project->height * p_ctx.stitch_width);

        draw_chart(page_content_ctx, p_ctx, backstitch_only);
        save_page(page, page_content_ctx, p_ctx.chart_x);
    } else {
        // tile pages
        for (int y = 0; y < _pattern_no_pages_tall; y++) {
            for (int x = 0; x < _pattern_no_pages_wide; x++) {
                create_chart_page(x, y, backstitch_only);
            }
        }
    }
}

void PDFWizard::create_chart_page(int page_x, int page_y, bool backstitch_only) {
    PDFPage *page = new PDFPage();
    page->SetMediaBox(PDFRectangle(0, 0, A4_WIDTH, A4_HEIGHT));
    PageContentContext *page_content_ctx = _pdf_writer.StartPageContentContext(page);

    PatternPageContext p_ctx;
    p_ctx.chart_x = CHART_X;
    p_ctx.chart_y = CHART_Y;
    p_ctx.x_start = page_x * CHART_STITCHES_X;
    p_ctx.y_start = std::max(_project->height - ((page_y + 1) * CHART_STITCHES_Y), 0);
    p_ctx.x_end = std::min(p_ctx.x_start + CHART_STITCHES_X, _project->width);
    p_ctx.y_end = p_ctx.y_start + CHART_STITCHES_Y;
    p_ctx.stitch_width = CHART_STITCH_WIDTH;

    // If the end point we calculate is greater than to the start point of the previous page, it's wrong
    if (p_ctx.y_end > _project->height - (page_y * CHART_STITCHES_Y))
        p_ctx.y_end = _project->height % CHART_STITCHES_Y;

    // If a section does not use all of CHART_STITCHES_Y, the vertical start point needs to be adjusted up
    int section_height = p_ctx.y_end - p_ctx.y_start;
    if (section_height < CHART_STITCHES_Y)
        p_ctx.chart_y = p_ctx.chart_y + ((CHART_STITCHES_Y - section_height) * CHART_STITCH_WIDTH);

    p_ctx.chart_x_end = p_ctx.chart_x + ((p_ctx.x_end - p_ctx.x_start) * CHART_STITCH_WIDTH);
    p_ctx.chart_y_end = p_ctx.chart_y + (section_height * CHART_STITCH_WIDTH);

    draw_chart(page_content_ctx, p_ctx, backstitch_only);
    save_page(page, page_content_ctx, p_ctx.chart_x);
}

void PDFWizard::save_page(PDFPage *page, PageContentContext *page_content_ctx, float margin_x) {
    std::time_t t = std::time(nullptr);
    std::tm *const pTInfo = std::localtime(&t);
    page_content_ctx->WriteText(margin_x, 40, fmt::format("© {} {}", _settings->author, 1900 + pTInfo->tm_year), *_body_small_text_options);

    std::string page_no = fmt::format("{}/{}", _current_page, _total_pages);
    auto text_dimensions = _helvetica->CalculateTextDimensions(page_no, 10);
    page_content_ctx->WriteText(A4_WIDTH - margin_x - text_dimensions.width, 40, page_no, *_body_small_text_options);
    _current_page++;

    if (_pdf_writer.EndPageContentContext(page_content_ctx) != eSuccess)
        throw std::runtime_error("Error ending page");

    if (_pdf_writer.WritePageAndRelease(page) != eSuccess)
        throw std::runtime_error("Error writing page");
}

void PDFWizard::fetch_symbol_data() {
    for (int i = 0; i < _project->palette.size(); i++) {
        Thread *t = _project->palette[i];
        if (t == nullptr)
            continue;

        _symbol_key_rows.push_back(new TableRow{
            i, t->full_name(t->default_position()),
            t->description(t->default_position()), t->is_blended()
        });
    }

    // Find stitch count for each colour
    for (int x = 0; x < _project->width; x++) {
        for (int y = 0; y < _project->height; y++) {
            int palette_id = _project->thread_data[x][y];
            if (palette_id == -1)
                continue;

            for (TableRow *row : _symbol_key_rows) {
                if (row->palette_id == palette_id) {
                    row->no_stitches++;
                    break;
                }
            }
        }
    }

    // Find backstitch count for each colour
    for (BackStitch bs : _project->backstitches) {
        for (TableRow *row : _symbol_key_rows) {
            if (row->palette_id == bs.palette_index) {
                row->no_backstitches++;
            }
        }
    }

    // Remove palette items with no stitches
    std::vector<int> to_delete;
    for (int i = 0; i < _symbol_key_rows.size(); i++) {
        if (_symbol_key_rows[i]->no_stitches == 0 && _symbol_key_rows[i]->no_backstitches == 0)
            to_delete.push_back(i);
    }
    for (auto rit = to_delete.rbegin(); rit != to_delete.rend(); rit++)
        _symbol_key_rows.erase(_symbol_key_rows.begin() + *rit);

    if (_symbol_key_rows.size() > MAX_SYMBOLS)
        throw std::runtime_error("The project palette is too large for the number of symbols available.");

    std::string resources_dir = get_resources_dir();
    if (resources_dir == "")
        throw std::runtime_error("Couldn't fetch resource directory");

    // Load all symbols
    int i = 0;
    for (TableRow *row : _symbol_key_rows) {
        if (row->no_stitches != 0) {
            std::string tint = ".png";

            if (_settings->render_in_colour) {
                Thread *t = _project->palette[row->palette_id];
                if (((t->R * 0.299f) + (t->G * 0.587f) + (t->B * 0.114f)) <= 186.f)
                    tint = "-white.png";
            }

            _project_symbols[row->palette_id] = resources_dir + "/symbols/" + symbols[i] + tint;
            i++;
        }
    }

    if (_settings->render_in_colour || _project->backstitches.size() == 0)
        return;

    i = 0;
    for (TableRow *row : _symbol_key_rows) {
        if (row->no_backstitches != 0 && _settings->render_in_colour == false) {
            _project_dashpatterns[row->palette_id] = dash_patterns[i];
            i++;
        }
    }
}