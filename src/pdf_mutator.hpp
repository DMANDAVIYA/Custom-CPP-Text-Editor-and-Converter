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

            // TODO: Iterate over the pages and text objects to find matches 
            // from the modifications JSON, swap the strings, and write back.

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
