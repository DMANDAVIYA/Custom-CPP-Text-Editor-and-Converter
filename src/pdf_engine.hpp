#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <mupdf/fitz.h>
#include <nlohmann/json.hpp>

namespace pdfmaker {

class PdfEngine {
public:
    PdfEngine() {
        ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
        if (!ctx) {
            throw std::runtime_error("Cannot initialize MuPDF context");
        }
        fz_register_document_handlers(ctx);
    }

    ~PdfEngine() {
        fz_drop_context(ctx);
    }

    fz_context* get_ctx() {
        return ctx;
    }

    int get_page_count(const std::string& filename) {
        fz_document *doc = NULL;
        int page_count = 0;
        try {
            doc = fz_open_document(ctx, filename.c_str());
            page_count = fz_count_pages(ctx, doc);
        } catch (...) {
            std::cerr << "MuPDF count pages error" << std::endl;
        }
        if (doc) fz_drop_document(ctx, doc);
        return page_count;
    }

    nlohmann::json extract_text_blocks(const std::string& filename, int page_num) {
        nlohmann::json blocks = nlohmann::json::array();
        
        fz_document *doc = NULL;
        fz_page *page = NULL;
        fz_stext_page *text = NULL;
        fz_device *dev = NULL;

        try {
            doc = fz_open_document(ctx, filename.c_str());
            int page_count = fz_count_pages(ctx, doc);
            if (page_num < 0 || page_num >= page_count) {
                std::cerr << "Page out of range" << std::endl;
                goto cleanup;
            }

            page = fz_load_page(ctx, doc, page_num);
            
            text = fz_new_stext_page(ctx, fz_bound_page(ctx, page));
            dev = fz_new_stext_device(ctx, text, NULL);
            
            fz_run_page(ctx, page, dev, fz_identity, NULL);
            fz_close_device(ctx, dev);
            fz_drop_device(ctx, dev);
            dev = NULL;

            for (fz_stext_block *block = text->first_block; block; block = block->next) {
                if (block->type != FZ_STEXT_BLOCK_TEXT) continue;
                
                for (fz_stext_line *line = block->u.t.first_line; line; line = line->next) {
                    fz_rect bbox = line->bbox;
                    
                    std::string line_text = "";
                    for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
                       char buf[10];
                       int n = fz_runetochar(buf, ch->c);
                       line_text.append(buf, n);
                    }

                    nlohmann::json line_json = {
                        {"text", line_text},
                        {"x", bbox.x0},
                        {"y", bbox.y0},
                        {"width", bbox.x1 - bbox.x0},
                        {"height", bbox.y1 - bbox.y0}
                    };
                    blocks.push_back(line_json);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "MuPDF Extraction Error: " << e.what() << std::endl;
        }
        
cleanup:
        if (dev) fz_drop_device(ctx, dev);
        if (text) fz_drop_stext_page(ctx, text);
        if (page) fz_drop_page(ctx, page);
        if (doc) fz_drop_document(ctx, doc);

        return blocks;
    }

    bool render_page_to_png(const std::string& filename, int page_num, const std::string& output_png, float zoom = 2.0f) {
        fz_document *doc = NULL;
        fz_page *page = NULL;
        fz_pixmap *pix = NULL;
        bool success = false;
        
        try {
            doc = fz_open_document(ctx, filename.c_str());
            page = fz_load_page(ctx, doc, page_num);
            
            fz_matrix ctm;
            ctm = fz_scale(zoom, zoom);
            
            pix = fz_new_pixmap_from_page_number(ctx, doc, page_num, ctm, fz_device_rgb(ctx), 0);
            fz_save_pixmap_as_png(ctx, pix, output_png.c_str());
            success = true;
        } catch(const std::exception& e) {
            std::cerr << "MuPDF Render Error: " << e.what() << std::endl;
        }

        if (pix) fz_drop_pixmap(ctx, pix);
        if (page) fz_drop_page(ctx, page);
        if (doc) fz_drop_document(ctx, doc);
        
        return success;
    }

private:
    fz_context *ctx;
};

} // namespace pdfmaker
