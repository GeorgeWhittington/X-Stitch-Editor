#pragma once
#include <vector>
#include <map>
#include <PDFWriter.h>
#include <PDFPage.h>
#include <PDFUsedFont.h>
#include <PageContentContext.h>
#include "project.hpp"

struct PDFSettings {
    bool render_in_colour;
    bool render_backstitch_chart;
    std::string author;
};

struct TableRow {
    int palette_id;
    std::string number;
    std::string description;
    int no_stitches = 0;
    int no_backstitches = 0;
};

typedef std::vector<double> DashPattern;

void draw_line(PageContentContext *ctx, float x1, float y1, float x2, float y2);

class PDFWizard {
public:
    PDFWizard(Project *project, PDFSettings *settings);
    void create_and_save_pdf(std::string path);

private:
    void create_title_page();
    void create_symbol_key_page();
    void create_chart_pages(bool backstitch_only);
    void create_chart_page(int page_x, int page_y, bool backstitch_only);
    void save_page(PDFPage *page, PageContentContext *page_content_ctx, float margin_x);
    void draw_gridlines(PageContentContext *ctx, int x_start, int x_end, int y_start, int y_end,
                        float chart_x, float chart_x_end, float chart_y, float chart_y_end, float stitch_width);
    void draw_backstitches(PageContentContext *ctx, int x_start, int x_end, int y_start, int y_end,
                           float chart_x, float chart_x_end, float chart_y, float chart_y_end, float stitch_width, bool over_symbols);
    void draw_chart(PageContentContext *ctx, int x_start, int x_end, int y_start, int y_end,
                   float chart_x, float chart_x_end, float chart_y, float chart_y_end, float stitch_width, bool backstitch_only);

    void fetch_symbol_data();

    Project *_project;
    PDFSettings *_settings;
    int _pattern_no_pages_wide;
    int _pattern_no_pages_tall;
    int _total_pages;
    int _current_page = 1;

    PDFWriter _pdf_writer;
    PDFUsedFont *_helvetica;
    PDFUsedFont *_helvetica_bold;
    std::vector<TableRow*> _symbol_key_rows;
    std::map<int, std::string> _project_symbols;
    std::map<int, DashPattern> _project_dashpatterns;
    AbstractContentContext::TextOptions *_title_text_options;
    AbstractContentContext::TextOptions *_body_text_options;
    AbstractContentContext::TextOptions *_body_bold_text_options;
    AbstractContentContext::TextOptions *_body_small_text_options;
};