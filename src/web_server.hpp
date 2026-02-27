#pragma once
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include "pdf_engine.hpp"
#include "pdf_mutator.hpp"

namespace pdfmaker {

class WebServer {
public:
    WebServer(const std::string& host, int port) : host_(host), port_(port) {
        setup_routes();
    }

    void start() {
        std::cout << "SYS_STATUS: C++ BACKEND ONLINE ON HTTP://" << host_ << ":" << port_ << std::endl;
        svr_.listen(host_.c_str(), port_);
    }

private:
    httplib::Server svr_;
    std::string host_;
    int port_;
    PdfEngine pdf_engine_;

    void setup_routes() {
        // Health check
        svr_.Get("/api/health", [](const httplib::Request& req, httplib::Response& res) {
            nlohmann::json response = {{"status", "ok"}, {"engine", "C++ Canvas Projection Override"}};
            res.set_content(response.dump(), "application/json");
        });

        // Serve static files
        svr_.set_mount_point("/", "../web");

        // Parse PDF into JSON coordinates
        svr_.Post("/api/pdf/extract", [this](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_file("file")) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "No file uploaded"}}).dump(), "application/json");
                return;
            }

            auto file = req.get_file_value("file");
            std::string temp_path = "/tmp/temp_extract.pdf";
            
            std::ofstream out(temp_path, std::ios::binary);
            out.write(file.content.data(), file.content.length());
            out.close();
            
            int page_num = 0;
            if (req.has_file("page")) {
                page_num = std::stoi(req.get_file_value("page").content);
            }

            try {
                int total_pages = pdf_engine_.get_page_count(temp_path);
                nlohmann::json blocks = pdf_engine_.extract_text_blocks(temp_path, page_num);
                nlohmann::json response = {
                    {"status", "success"}, 
                    {"total_pages", total_pages},
                    {"page", page_num},
                    {"blocks", blocks}
                };
                res.set_content(response.dump(), "application/json");
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
            }
        });

        // Render PDF page to PNG
        svr_.Post("/api/pdf/render", [this](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_file("file")) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "No file uploaded"}}).dump(), "application/json");
                return;
            }

            auto file = req.get_file_value("file");
            std::string temp_path = "/tmp/temp_render.pdf";
            std::string output_png = "/tmp/out.png";
            
            int page_num = 0;
            if (req.has_file("page")) {
                page_num = std::stoi(req.get_file_value("page").content);
            }
            
            std::ofstream out(temp_path, std::ios::binary);
            out.write(file.content.data(), file.content.length());
            out.close();

            if (pdf_engine_.render_page_to_png(temp_path, page_num, output_png)) {
                std::ifstream in(output_png, std::ios::binary);
                std::string png_data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                res.set_content(png_data, "image/png");
            } else {
                res.status = 500;
                res.set_content(nlohmann::json({{"error", "Render failed"}}).dump(), "application/json");
            }
        });

        // Compile PDF mutations and return new generated PDF
        svr_.Post("/api/pdf/compile", [this](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_file("file") || !req.has_file("modifications")) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "Missing file or modifications"}}).dump(), "application/json");
                return;
            }

            auto file = req.get_file_value("file");
            auto mod_str = req.get_file_value("modifications").content;
            
            nlohmann::json modifications;
            try {
                modifications = nlohmann::json::parse(mod_str);
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", std::string("Invalid JSON parsing: ") + e.what()}}).dump(), "application/json");
                return;
            }

            std::string temp_input = "/tmp/temp_compile_in.pdf";
            std::string temp_output = "/tmp/temp_compile_out.pdf";
            
            std::ofstream out(temp_input, std::ios::binary);
            out.write(file.content.data(), file.content.length());
            out.close();

            PdfMutator mutator(pdf_engine_.get_ctx());
            if (mutator.mutate_document(temp_input, temp_output, modifications)) {
                std::ifstream in(temp_output, std::ios::binary);
                std::string pdf_data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                res.set_content(pdf_data, "application/pdf");
                res.set_header("Content-Disposition", "attachment; filename=\"modified.pdf\"");
            } else {
                res.status = 500;
                res.set_content(nlohmann::json({{"error", "Mutation engine failed to process the PDF"}}).dump(), "application/json");
            }
        });
    }
};

} // namespace pdfmaker
