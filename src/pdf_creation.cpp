#include <iostream>
#include <map>
#include <nanogui/nanogui.h>
#include <freetype/ftstroke.h>
#include "pdf_creation.hpp"
#include "threads.hpp"

const float A4_WIDTH = 595.f;
const float A4_HEIGHT = 842.f;
const float PAGE_MARGIN = 60.f;

const std::string symbol_key_headers[4] = {"Symbol", "Number", "Description", "N.O. Stitches"};

const std::string symbol_dir = "/Users/george/Documents/uni_year_three/Digital Systems Project/X-Stitch-Editor/assets/symbols/";
const std::vector<std::string> symbols = {
    "air.png", "aircraft.png", "arrow-down.png", "arrow-left.png", "arrow-up.png",
    "arrow-right.png", "arrow-with-circle-down.png", "arrow-with-circle-left.png",
    "arrow-with-circle-right.png", "arrow-with-circle-up.png", "attachment.png",
    "beamed-note.png", "bell.png", "book.png", "bug.png", "check.png",
    "chevron-down.png", "chevron-left.png", "chevron-right.png", "chevron-up.png",
    "circle-with-cross.png", "circle-with-minus.png", "circle-with-plus.png",
    "circle.png", "cloud.png", "controller-fast-backward.png", "controller-fast-forward.png"
}; // TODO: add the rest

PDFWizard::PDFWizard(Project *project) : _project(project) {};

void PDFWizard::create_and_save_pdf(std::string path) {
    if (_pdf_writer.StartPDF(
        path, ePDFVersion13,
        LogConfiguration(true, true, "/Users/george/Documents/uni_year_three/Digital Systems Project/X-Stitch-Editor/assets/pdf-debug.txt")
    ) != eSuccess)
        throw std::runtime_error("Error creating PDFWriter");

    _helvetica = _pdf_writer.GetFontForFile("/Users/george/Documents/uni_year_three/Digital Systems Project/X-Stitch-Editor/assets/Helvetica.ttf");
    if (!_helvetica)
        throw std::runtime_error("Error loading font Helvetica for PDF generation");
    _helvetica_bold = _pdf_writer.GetFontForFile("/Users/george/Documents/uni_year_three/Digital Systems Project/X-Stitch-Editor/assets/Helvetica-Bold.ttf");
    if (!_helvetica_bold)
        throw std::runtime_error("Error loading font Helvetica-Bold for PDF generation");

    auto title_text_options = AbstractContentContext::TextOptions(_helvetica, 24, AbstractContentContext::eGray, 0);
    auto body_text_options = AbstractContentContext::TextOptions(_helvetica, 14, AbstractContentContext::eGray, 0);
    auto body_bold_text_options = AbstractContentContext::TextOptions(_helvetica_bold, 14, AbstractContentContext::eGray, 0);
    auto grid_text_options = AbstractContentContext::TextOptions(_helvetica, 10, AbstractContentContext::eGray, 0);
    _title_text_options = &title_text_options;
    _body_text_options = &body_text_options;
    _body_bold_text_options = &body_bold_text_options;
    _grid_text_options = &grid_text_options;

    fetch_symbol_data();

    create_title_page();
    create_symbol_key_page();
    create_chart_pages();

    if (_pdf_writer.EndPDF() != eSuccess)
        throw std::runtime_error("Error writing PDF file");
}

void PDFWizard::create_title_page() {
    PDFPage *page = new PDFPage();
    page->SetMediaBox(PDFRectangle(0, 0, A4_WIDTH, A4_HEIGHT));
    PageContentContext *page_content_ctx = _pdf_writer.StartPageContentContext(page);

    page_content_ctx->WriteText(PAGE_MARGIN, A4_HEIGHT - PAGE_MARGIN - 10, _project->title, *_title_text_options);

    // Calculate pattern preview scale
    float page_width = A4_WIDTH - (PAGE_MARGIN * 2);
    float page_height = A4_HEIGHT - (PAGE_MARGIN * 2) - 40;  // subtract 60 more if subtitle section is being rendered
    float page_x = PAGE_MARGIN;
    float page_y = PAGE_MARGIN; // add 60 more if subtitle section is rendered
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

    page_content_ctx->w(1.0); // line width
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
            page_content_ctx->m(x_start, y_end); // move
            page_content_ctx->l(x_end, y_end); // draw line to
            page_content_ctx->S(); // stroke

            // Draw line from tr to br
            page_content_ctx->m(x_end, y_end); // move
            page_content_ctx->l(x_end, y_start); // draw line to
            page_content_ctx->S(); // stroke
        }
    }

    // Stroke pattern outline
    page_content_ctx->w(2.0); // line width
    page_content_ctx->RG(0.f, 0.f, 0.f); // set stroke colour

    page_content_ctx->re(page_x, page_y, page_width, page_height); // draw rectangle
    page_content_ctx->S(); // stroke

    // TODO: render subtitle section (if it exists)

    // TODO: draw copyright + page no in footer (write a function that does this to each page)

    save_page(page, page_content_ctx);
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

        auto desc_text_dimensions = _helvetica->CalculateTextDimensions(row->description, 14);
        if (desc_text_dimensions.width > longest_in_column[2])
            longest_in_column[2] = desc_text_dimensions.width;
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
    page_content_ctx->m(PAGE_MARGIN, y); // move
    page_content_ctx->l(A4_WIDTH - PAGE_MARGIN, y); // draw line to
    page_content_ctx->S(); // stroke

    x = PAGE_MARGIN;
    y -= 15;

    AbstractContentContext::ImageOptions image_options;
    image_options.transformationMethod = AbstractContentContext::eFit;
    image_options.boundingBoxWidth = 14.f;
    image_options.boundingBoxHeight = 14.f;

    nanogui::Color c;

    for (TableRow *row : _symbol_key_rows) {
        c = _project->palette[row->palette_id]->color();
        page_content_ctx->rg(c.r(), c.g(), c.b()); // set fill colour
        page_content_ctx->re(x, y - 6, 18.f, 18.f); // draw rectangle
        page_content_ctx->f(); // fill

        page_content_ctx->w(0.5f); // set line width
        page_content_ctx->RG(0.f, 0.f, 0.f); // set stroke colour
        page_content_ctx->re(x + 20, y - 6, 18.f, 18.f); // draw rectangle
        page_content_ctx->S(); // stroke

        page_content_ctx->DrawImage(x + 22, y - 4, _project_symbols[row->palette_id], image_options);
        x += longest_in_column[0] + column_spacing;

        page_content_ctx->WriteText(x, y, row->number, *_body_text_options);
        x += longest_in_column[1] + column_spacing;

        page_content_ctx->WriteText(x, y, row->description, *_body_text_options);
        x += longest_in_column[2] + column_spacing;

        page_content_ctx->WriteText(x, y, std::to_string(row->no_stitches), *_body_text_options);

        x = PAGE_MARGIN;
        y -= 20;
    }

    save_page(page, page_content_ctx);
}

const float CHART_WIDTH = A4_WIDTH - 95.f; // 500 -> 50 stitches
const float CHART_HEIGHT = A4_HEIGHT - 102.f; // 740 -> 74 stitches
const float CHART_X = 47.5f;
const float CHART_Y = 51.f;
const float CHART_STITCH_WIDTH = 10.f;
const int CHART_STITCHES_X = CHART_WIDTH / CHART_STITCH_WIDTH;
const int CHART_STITCHES_Y = CHART_HEIGHT / CHART_STITCH_WIDTH;

void set_minor_grid_options(PageContentContext *page_content_ctx) {
    page_content_ctx->J(FT_Stroker_LineCap_::FT_STROKER_LINECAP_SQUARE); // set end cap
    page_content_ctx->w(0.35f); // line width
    page_content_ctx->RG(0.6f, 0.6f, 0.6f); // set stroke colour
}

void draw_line(PageContentContext *ctx, float x1, float y1, float x2, float y2) {
    ctx->m(x1, y1); // move
    ctx->l(x2, y2); // draw line to
    ctx->S(); // stroke
}

void PDFWizard::draw_grid(PageContentContext *ctx, int x_start, int x_end, int y_start, int y_end,
                          float chart_x, float chart_x_end, float chart_y, float chart_y_end, float stitch_width) {
    AbstractContentContext::ImageOptions image_options;
    image_options.transformationMethod = AbstractContentContext::eFit;
    image_options.boundingBoxWidth = stitch_width - 3.f;
    image_options.boundingBoxHeight = stitch_width - 3.f;
    set_minor_grid_options(ctx);
    for (int x = x_start; x < x_end; x++) {
        int rel_x = x_start != 0 ? x - x_start : x;
        float pos_x = chart_x + (rel_x * stitch_width);

        // Draw minor vertical grid lines
        if (x > x_start)
            draw_line(ctx, pos_x, chart_y, pos_x, chart_y_end);

        for (int y = y_start; y < y_end; y++) {
            int rel_y = y_start != 0 ? y - y_start : y;
            float pos_y = chart_y + (rel_y * stitch_width);

            // Draw minor horizontal grid lines
            if (x == x_start && y > y_start)
                draw_line(ctx, chart_x, pos_y, chart_x_end, pos_y);

            int palette_index = _project->thread_data[x][y];
            if (palette_index == -1)
                continue;
            // TODO: if drawing colour bg draw symbol in black or white depending on:
            // if (red*0.299 + green*0.587 + blue*0.114) > 186 use #000000 else use #ffffff
            // probably work this out in the preprocessing step!
            ctx->DrawImage(pos_x + 1.5f, pos_y + 1.5f, _project_symbols[palette_index], image_options);
        }
    }

    // Draw major gridlines and line no's
    ctx->w(1.f); // line width
    ctx->RG(0.f, 0.f, 0.f); // set stroke colour
    for (int x = x_start; x <= x_end; x++) {
        if (x % 10 != 0)
            continue;

        int rel_x = x_start != 0 ? x - x_start : x;
        float pos_x = chart_x + (rel_x * stitch_width);
        draw_line(ctx, pos_x, chart_y, pos_x, chart_y_end);

        // Write line no
        std::string x_str = std::to_string(x);
        float start = pos_x - 20.f;
        auto x_text_dimensions = _helvetica->CalculateTextDimensions(x_str, 10);
        float offset = (40.f - x_text_dimensions.width) / 2.f;
        ctx->WriteText(start + offset, chart_y_end + 5.f, x_str, *_grid_text_options);

        for (int y = y_start; y <= y_end; y++) {
            int y_inverted = _project->height - y;
            if (y_inverted % 10 != 0 || y_inverted == 0)
                continue;

            int rel_y = y_start != 0 ? y - y_start : y;
            float pos_y = chart_y + (rel_y * stitch_width);
            draw_line(ctx, chart_x, pos_y, chart_x_end, pos_y);

            // Write line no
            std::string y_str = std::to_string(y_inverted);
            float start = pos_y - 20.f;
            auto y_text_dimensions = _helvetica->CalculateTextDimensions(y_str, 10);
            float offset = (40.f - y_text_dimensions.height) / 2.f;
            ctx->WriteText(chart_x - y_text_dimensions.width - 5.f, start + offset, y_str, *_grid_text_options);
        }
    }

    ctx->w(1.5f); // line width
    ctx->RG(0.f, 0.f, 0.f); // set stroke colour
    ctx->re(chart_x, chart_y, (x_end - x_start) * stitch_width, (y_end - y_start) * stitch_width); // draw rectangle
    ctx->S(); // stroke
}

void PDFWizard::create_chart_pages() {
    int no_pages_wide = std::ceil((float)_project->width / (float)CHART_STITCHES_X);
    int no_pages_tall = std::ceil((float)_project->height / (float)CHART_STITCHES_Y);

    if (no_pages_wide == 1 && no_pages_tall == 1) {
        // special case, scale to fit max width/height (similar to preview)
        PDFPage *page = new PDFPage();
        page->SetMediaBox(PDFRectangle(0, 0, A4_WIDTH, A4_HEIGHT));
        PageContentContext *page_content_ctx = _pdf_writer.StartPageContentContext(page);

        float chart_width = CHART_WIDTH;
        float chart_height = CHART_HEIGHT;
        float chart_x = CHART_X;
        float chart_y = CHART_Y;
        float stitch_width;

        // Scale by different axis depending on which aspect ratio is larger
        if ((chart_height / chart_width) >= ((float)_project->height / (float)_project->width)) {
            stitch_width = chart_width / _project->width;
            float new_page_height = stitch_width * _project->height;
            float vertical_margin = (chart_height - new_page_height) / 2;
            chart_height = new_page_height;
            chart_y += vertical_margin;
        } else {
            stitch_width = chart_height / _project->height;
            float new_page_width = stitch_width * _project->width;
            float horizontal_margin = (chart_width - new_page_width) / 2;
            chart_width = new_page_width;
            chart_x += horizontal_margin;
        }

        float chart_x_end = chart_x + (_project->width * stitch_width);
        float chart_y_end = chart_y + (_project->height * stitch_width);

        draw_grid(page_content_ctx, 0, _project->width, 0, _project->height,
                  chart_x, chart_x_end, chart_y, chart_y_end, stitch_width);
        save_page(page, page_content_ctx);
    } else {
        // tile pages
        for (int y = 0; y < no_pages_tall; y++) {
            for (int x = 0; x < no_pages_wide; x++) {
                create_chart_page(x, y);
            }
        }

    }
}



void PDFWizard::create_chart_page(int page_x, int page_y) {
    PDFPage *page = new PDFPage();
    page->SetMediaBox(PDFRectangle(0, 0, A4_WIDTH, A4_HEIGHT));
    PageContentContext *page_content_ctx = _pdf_writer.StartPageContentContext(page);

    float chart_x = CHART_X;
    float chart_y = CHART_Y;

    int x_start = page_x * CHART_STITCHES_X;
    int y_start = std::max(_project->height - ((page_y + 1) * CHART_STITCHES_Y), 0);

    int x_end = std::min(x_start + CHART_STITCHES_X, _project->width - 1);
    int y_end = y_start + CHART_STITCHES_Y;

    // If the end point we calculate is greater than to the start point of the previous page, it's wrong
    if (y_end > _project->height - (page_y * CHART_STITCHES_Y))
        y_end = _project->height % CHART_STITCHES_Y;

    // If a section does not use all of CHART_STITCHES_Y, the vertical start point needs to be adjusted up
    int section_height = y_end - y_start;
    if (section_height < CHART_STITCHES_Y)
        chart_y = chart_y + ((CHART_STITCHES_Y - section_height) * CHART_STITCH_WIDTH);

    float chart_x_end = chart_x + ((x_end - x_start) * CHART_STITCH_WIDTH);
    float chart_y_end = chart_y + (section_height * CHART_STITCH_WIDTH);

    draw_grid(page_content_ctx, x_start, x_end, y_start, y_end,
              chart_x, chart_x_end, chart_y, chart_y_end, CHART_STITCH_WIDTH);
    save_page(page, page_content_ctx);
}

void PDFWizard::save_page(PDFPage *page, PageContentContext *page_content_ctx) {
    if (_pdf_writer.EndPageContentContext(page_content_ctx) != eSuccess)
        throw std::runtime_error("Error ending page");

    if (_pdf_writer.WritePageAndRelease(page) != eSuccess)
        throw std::runtime_error("Error writing page");
}

void PDFWizard::fetch_symbol_data() {
    for (int i = 0; i < _project->palette.size(); i ++) {
        Thread *t = _project->palette[i];
        _symbol_key_rows.push_back(new TableRow{i, t->company + " " + t->number, t->description, 0});
    }

    // Find stitch count for each colour
    for (int x = 0; x < _project->width; x++) {
        for (int y = 0; y < _project->height; y++) {
            int palette_id = _project->thread_data[x][y];
            if (palette_id == -1)
                continue;

            _symbol_key_rows[palette_id]->no_stitches++;
        }
    }

    // Remove palette items with no stitches
    std::vector<int> to_delete;
    for (int i = 0; i < _symbol_key_rows.size(); i++) {
        if (_symbol_key_rows[i]->no_stitches == 0)
            to_delete.push_back(i);
    }
    for (auto rit = to_delete.rbegin(); rit != to_delete.rend(); rit++)
        _symbol_key_rows.erase(_symbol_key_rows.begin() + *rit);

    // Load all symbols
    int i = 0;
    for (TableRow *row : _symbol_key_rows) {
        _project_symbols[row->palette_id] = symbol_dir + symbols[i];
        i++;
    }
}