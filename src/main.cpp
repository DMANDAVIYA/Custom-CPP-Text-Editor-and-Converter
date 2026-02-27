#include <iostream>
#include "web_server.hpp"

int main() {
    std::cout << "INITIALIZING: TERMINAL DOC_CONVERTER C++ ENGINE..." << std::endl;
    
    pdfmaker::WebServer server("0.0.0.0", 8000);
    server.start();

    return 0;
}
