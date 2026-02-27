#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

namespace pdfmaker {

class PdfMutator {
public:
    PdfMutator(fz_context* ctx) : ctx_(ctx) {}

    // Main entry point for the compile command
    bool mutate_document(const std::string& input_pdf, const std::string& output_pdf, const nlohmann::json& modifications) {
        pdf_document *doc = nullptr;
        bool success = false;

        try {
            doc = pdf_open_document(ctx_, input_pdf.c_str());
            if (!doc) throw std::runtime_error("Failed to open PDF for mutation");

            // Iterate over modifications array
            for (const auto& mod : modifications) {
                int page_index = mod["page"];
                std::string originalStr = mod["orig"];
                std::string newStr = mod["text"];

                pdf_obj *page_obj = pdf_lookup_page_obj(ctx_, doc, page_index);
                if (!page_obj) continue;

                pdf_obj *contents = pdf_dict_get(ctx_, page_obj, PDF_NAME(Contents));
                if (!contents) continue;

                // Load the decoded content stream buffer
                fz_buffer *buf = pdf_load_stream(ctx_, contents);
                if (!buf) continue;

                unsigned char *data = nullptr;
                size_t len = fz_buffer_storage(ctx_, buf, &data);
                if (data && len > 0) {
                    std::string content_str(reinterpret_cast<char*>(data), len);
                    
                    // Naive Byte Stream Mutation: finding exact PDF String Objects (text)
                    // and modifying them inline without touching anything else!
                    std::string escaped_orig = "(" + originalStr + ")";
                    std::string escaped_new = "(" + newStr + ")";

                    size_t pos = 0;
                    bool modified = false;
                    while ((pos = content_str.find(escaped_orig, pos)) != std::string::npos) {
                        content_str.replace(pos, escaped_orig.length(), escaped_new);
                        pos += escaped_new.length();
                        modified = true;
                    }

                    // If we made a successful mutation, write the buffer back into the PDF tree
                    if (modified) {
                        fz_buffer* new_buf = fz_new_buffer_from_copied_data(ctx_, reinterpret_cast<const unsigned char*>(content_str.data()), content_str.length());
                        pdf_update_stream(ctx_, doc, contents, new_buf, 0);
                        fz_drop_buffer(ctx_, new_buf);
                    }
                }
                fz_drop_buffer(ctx_, buf);
            }

            // Save the altered file
            pdf_write_options write_opts = {0};
            pdf_save_document(ctx_, doc, output_pdf.c_str(), &write_opts);
            success = true;

        } catch (const std::exception& e) {
            std::cerr << "Mutation Engine Error: " << e.what() << std::endl;
        }

        if (doc) pdf_drop_document(ctx_, doc);
        return success;
    }

private:
    fz_context* ctx_;
};

} // namespace pdfmaker
