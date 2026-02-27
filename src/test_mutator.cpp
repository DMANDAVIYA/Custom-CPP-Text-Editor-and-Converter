#include "pdf_engine.hpp"
#include "pdf_mutator.hpp"
#include <iostream>

int main() {
    try {
        pdfmaker::PdfEngine engine;
        pdfmaker::PdfMutator mutator(engine.get_ctx());
        nlohmann::json mod = {{
            {"page", 0},
            {"orig", "UCSD"},
            {"text", "UCD"},
            {"x", 50},
            {"y", 50},
            {"width", 50},
            {"height", 16}
        }};
        std::cout << "Starting mutation..." << std::endl;
        bool res = mutator.mutate_document("../sample.pdf", "out.pdf", mod);
        std::cout << "Mutation finished: " << res << std::endl;
    } catch(const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
