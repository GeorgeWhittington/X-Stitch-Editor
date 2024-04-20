#pragma once
#include <vector>
#include <map>
#include <PDFWriter.h>
#include <PDFPage.h>
#include <PDFUsedFont.h>
#include <PageContentContext.h>
#include "project.hpp"

struct TableRow {
    int palette_id;
    std::string number;
    std::string description;
    int no_stitches;
};

void set_minor_grid_options(PageContentContext *page_content_ctx);
void draw_line(PageContentContext *ctx, float x1, float y1, float x2, float y2);

class PDFWizard {
public:
    PDFWizard(Project *project);
    void create_and_save_pdf(std::string path);

private:
    void create_title_page();
    void create_symbol_key_page();
    void create_chart_pages();
    void create_chart_page(int page_x, int page_y);
    void save_page(PDFPage *page, PageContentContext *page_content_ctx);
    void draw_grid(PageContentContext *ctx, int x_start, int x_end, int y_start, int y_end,
                   float chart_x, float chart_x_end, float chart_y, float chart_y_end, float stitch_width);

    void fetch_symbol_data();

    PDFWriter _pdf_writer;
    PDFUsedFont *_helvetica;
    PDFUsedFont *_helvetica_bold;
    Project *_project;
    std::vector<TableRow*> _symbol_key_rows;
    std::map<int, std::string> _project_symbols;
    AbstractContentContext::TextOptions *_title_text_options;
    AbstractContentContext::TextOptions *_body_text_options;
    AbstractContentContext::TextOptions *_body_bold_text_options;
    AbstractContentContext::TextOptions *_grid_text_options;
};