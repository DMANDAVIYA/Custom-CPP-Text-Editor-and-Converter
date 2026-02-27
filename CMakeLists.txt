cmake_minimum_required(VERSION 3.14)
project(pdfmaker_engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Fetch cpp-httplib (lightweight header-only HTTP server)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.14.1
)
FetchContent_MakeAvailable(httplib)

# 2. Find system packages (MuPDF, OpenSSL, nlohmann-json)
find_package(OpenSSL REQUIRED)

# nlohmann-json is installed via apt in Dockerfile
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    message(WARNING "nlohmann/json.hpp not found in system, downloading...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# We need pkg-config for MuPDF
find_package(PkgConfig REQUIRED)
pkg_check_modules(MUPDF REQUIRED mupdf)

add_executable(pdfmaker_engine
    src/main.cpp
)

# Includes
target_include_directories(pdfmaker_engine PRIVATE 
    src/
    ${MUPDF_INCLUDE_DIRS}
)

if(NLOHMANN_JSON_INCLUDE_DIR)
    target_include_directories(pdfmaker_engine PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})
else()
    target_link_libraries(pdfmaker_engine PRIVATE nlohmann_json::nlohmann_json)
endif()

# Linking
target_link_libraries(pdfmaker_engine PRIVATE
    httplib::httplib
    OpenSSL::SSL
    OpenSSL::Crypto
    ${MUPDF_LIBRARIES}
    mupdf-third
    freetype
    harfbuzz
    jbig2dec
    jpeg
    openjp2
    mujs
    gumbo
    z
)

# On Linux, httplib needs pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(pdfmaker_engine PRIVATE Threads::Threads)
