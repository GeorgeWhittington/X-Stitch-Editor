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
};