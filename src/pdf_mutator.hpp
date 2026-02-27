#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

namespace pdfmaker {

std::string escape_pdf_string(const std::string& str) {
    std::string out;
    out.reserve(str.length() * 2);
    for (char c : str) {
        if (c == '(' || c == ')' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

class PdfMutator {
public:
    PdfMutator(fz_context* ctx) : ctx_(ctx) {}

    // Main entry point for the compile command
    bool mutate_document(const std::string& input_pdf, const std::string& output_pdf, const nlohmann::json& modifications) {
        pdf_document *doc = nullptr;
        bool success = false;

        fz_try(ctx_) {
            doc = pdf_open_document(ctx_, input_pdf.c_str());
            if (!doc) fz_throw(ctx_, FZ_ERROR_GENERIC, "Failed to open PDF for mutation");

            // Iterate over modifications array
            for (const auto& mod : modifications) {
                int page_index = mod["page"];
                std::string originalStr = mod["orig"];
                std::string newStr = mod["text"];

                pdf_obj *page_obj = pdf_lookup_page_obj(ctx_, doc, page_index);
                if (!page_obj) continue;

                // Load page details to get the coordinate space boundaries
                fz_page *fzpage = fz_load_page(ctx_, (fz_document*)doc, page_index);
                fz_rect bounds = fz_bound_page(ctx_, fzpage);
                fz_drop_page(ctx_, fzpage);

                float page_ht = bounds.y1 - bounds.y0;
                float pdf_x = mod["x"];
                float pdf_y = mod["y"];
                // We need to invert the Y coordinate from top-left to Bottom-Left for PDF streams
                float inverted_y = page_ht - pdf_y - static_cast<float>(mod["height"]); 

                // 1. Inject a standard Helvetica font dictionary into the page resources so we can write real text regardless of original hex subsetting
                pdf_obj *res = pdf_dict_get(ctx_, page_obj, PDF_NAME(Resources));
                if (!res) {
                    res = pdf_new_dict(ctx_, doc, 1);
                    pdf_dict_put_drop(ctx_, page_obj, PDF_NAME(Resources), res);
                }
                pdf_obj *fonts = pdf_dict_get(ctx_, res, PDF_NAME(Font));
                if (!fonts) {
                    fonts = pdf_new_dict(ctx_, doc, 1);
                    pdf_dict_put_drop(ctx_, res, PDF_NAME(Font), fonts);
                }
                
                pdf_obj *helv = pdf_new_dict(ctx_, doc, 4);
                pdf_dict_puts_drop(ctx_, helv, "Type", pdf_new_name(ctx_, "Font"));
                pdf_dict_puts_drop(ctx_, helv, "Subtype", pdf_new_name(ctx_, "Type1"));
                pdf_dict_puts_drop(ctx_, helv, "BaseFont", pdf_new_name(ctx_, "Helvetica"));
                pdf_dict_puts_drop(ctx_, helv, "Encoding", pdf_new_name(ctx_, "WinAnsiEncoding"));
                pdf_dict_puts_drop(ctx_, fonts, "SysOverrideFont", helv);

                float r = mod.value("r", 1.0f);
                float g = mod.value("g", 1.0f);
                float b = mod.value("b", 1.0f);

                // 2. Append directly to the Page Content array to "whiteout" the old coords and write our new string on top
                fz_buffer *over_buf = fz_new_buffer(ctx_, 256);
                
                // Shrink the height of the erasure bounding box slightly to avoid clipping adjacent horizontal table borders
                float box_w = static_cast<float>(mod["width"]);
                float box_h = static_cast<float>(mod["height"]);
                float shrink_y = box_h * 0.15f; 
                float shrink_w = box_w * 0.05f;

                // Color-matched rectangle to erase old text seamlessly
                fz_append_printf(ctx_, over_buf, "q %f %f %f rg %f %f %f %g re f Q\n", 
                    r, g, b,
                    pdf_x + (shrink_w/2), inverted_y + shrink_y, 
                    box_w - shrink_w, box_h - (shrink_y * 1.5f));
                
                // New text override
                std::string escaped_new = escape_pdf_string(newStr);
                fz_append_printf(ctx_, over_buf, "q 0 0 0 rg BT /SysOverrideFont %f Tf %f %f Td (%s) Tj ET Q\n", 
                    box_h * 0.82f, // Approximate points from bounding box height
                    pdf_x, inverted_y + (box_h * 0.15f), // Baseline shift
                    escaped_new.c_str());

                // Create a stream object and attach the buffer
                pdf_obj *dummy_dict = pdf_new_dict(ctx_, doc, 0);
                pdf_obj *new_stream_obj = pdf_add_object_drop(ctx_, doc, dummy_dict);
                pdf_update_stream(ctx_, doc, new_stream_obj, over_buf, 0);

                pdf_obj *contents = pdf_dict_get(ctx_, page_obj, PDF_NAME(Contents));
                
                if (pdf_is_array(ctx_, contents)) {
                    pdf_array_push(ctx_, contents, new_stream_obj);
                } else {
                    pdf_obj *new_arr = pdf_new_array(ctx_, doc, 2);
                    pdf_array_push(ctx_, new_arr, contents);
                    pdf_array_push(ctx_, new_arr, new_stream_obj);
                    pdf_dict_put_drop(ctx_, page_obj, PDF_NAME(Contents), new_arr);
                }
                
                pdf_drop_obj(ctx_, new_stream_obj);
                fz_drop_buffer(ctx_, over_buf);
            }

            // Save the altered file
            pdf_write_options write_opts = {0};
            pdf_save_document(ctx_, doc, output_pdf.c_str(), &write_opts);
            success = true;

        } fz_catch(ctx_) {
            std::cerr << "Mutation Engine Error: " << fz_caught_message(ctx_) << std::endl;
        }

        if (doc) pdf_drop_document(ctx_, doc);
        return success;
    }

private:
    fz_context* ctx_;
};

} // namespace pdfmaker
