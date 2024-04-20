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
    _title_text_options = &title_text_options;
    _body_text_options = &body_text_options;
    _body_bold_text_options = &body_bold_text_options;

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

void PDFWizard::create_chart_pages() {
    int no_pages_wide = std::ceil((float)_project->width / (float)CHART_STITCHES_X);
    int no_pages_tall = std::ceil((float)_project->height / (float)CHART_STITCHES_Y);

    if (no_pages_wide == 1 && no_pages_tall == 1) {
        // special case, scale to fit max width/height (similar to preview)
    } else {
        // tile pages
        for (int y = 0; y < no_pages_tall; y++) {
            for (int x = 0; x < no_pages_wide; x++) {
                create_chart_page(x, y);
            }
        }

    }
}

void set_gridline_parameters(PageContentContext *page_content_ctx, int line_no) {
    if (line_no % 10 == 0) {
        page_content_ctx->w(1.5f); // line width
        page_content_ctx->RG(0.f, 0.f, 0.f); // set stroke colour
    } else {
        page_content_ctx->RG(0.3f, 0.3f, 0.3f); // set stroke colour
        if (line_no % 5 == 0) {
            page_content_ctx->w(1.f); // line width
        } else {
            page_content_ctx->w(0.5f); // line width
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

    AbstractContentContext::ImageOptions image_options;
    image_options.transformationMethod = AbstractContentContext::eFit;
    image_options.boundingBoxWidth = CHART_STITCH_WIDTH - 2.f;
    image_options.boundingBoxHeight = CHART_STITCH_WIDTH - 2.f;
    page_content_ctx->J(FT_Stroker_LineCap_::FT_STROKER_LINECAP_SQUARE); // set end cap

    for (int x = x_start; x < x_end; x++) {
        int rel_x = x - x_start;
        float pos_x = chart_x + (rel_x * CHART_STITCH_WIDTH);

        if (x > x_start) {
            // Draw vertical grid lines
            set_gridline_parameters(page_content_ctx, x);
            page_content_ctx->m(pos_x, chart_y); // move
            page_content_ctx->l(pos_x, chart_y_end); // draw line to
            page_content_ctx->S(); // stroke

            if (x % 10 == 0) {
                // Draw line no
            }
        }

        for (int y = y_start; y < y_end; y++) {
            int rel_y = y - y_start;
            float pos_y = chart_y + (rel_y * CHART_STITCH_WIDTH);

            // Draw horizontal grid lines
            if (x == x_start && y > y_start) {
                set_gridline_parameters(page_content_ctx, x);
                page_content_ctx->m(chart_x, pos_y); // move
                page_content_ctx->l(chart_x_end, pos_y); // draw line to
                page_content_ctx->S(); // stroke

                if (y % 10 == 0) {
                    // Draw line no
                }
            }

            // TODO: if drawing colour bg draw symbol in black or white depending on:
            // if (red*0.299 + green*0.587 + blue*0.114) > 186 use #000000 else use #ffffff
            // probably work this out in the preprocessing step!
            page_content_ctx->DrawImage(pos_x + 1.f, pos_y + 1.f, _project_symbols[_project->thread_data[x][y]], image_options);
        }
    }

    page_content_ctx->w(1.5f); // line width
    page_content_ctx->RG(0.f, 0.f, 0.f); // set stroke colour
    page_content_ctx->re(chart_x, chart_y, (x_end - x_start) * CHART_STITCH_WIDTH, section_height * CHART_STITCH_WIDTH); // draw rectangle
    page_content_ctx->S(); // stroke

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