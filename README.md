# SYS_OVERRIDE: Native C++ PDF Text Editor

A high-performance, containerized PDF editing engine and retro-themed web interface built entirely in C++. This application allows users to perform surgical, deep-byte edits on PDF documents directly from the browser, achieving **100% format and layout preservation**, even on heavily compressed or hex-subsetted PDFs.

##  Core Features

- **Direct Byte-Stream Mutation**: Edits are processed entirely in the backend by C++ manipulating the raw PDF instruction stream natively via MuPDF, bypassing corrupted or destructive PDF-to-HTML conversion tools.
- **Flawless Layout Preservation**: Automatically samples micro-level background color gradients and utilizes "whiteout box" matrix projection to eliminate old artifact text, then injects clean `/Helvetica` overriding strings.
- **Retro CRT Interface**: Pure Vanilla JavaScript and CSS frontend designed like a vintage CRT terminal, providing a clean, aesthetic, and blazing-fast editing experience.
- **Smart Text Sanitization**: Aggressive auto-handling of broken UTF-8 characters, smart-quotes, and misaligned bullets into standardized ISO-ASCII equivalents.
- **Zero-Dependency Deployment**: Completely containerized via Docker. No need to install heavy C++ compiler toolchains or complex library dependencies on the host machine.

## Technology Stack

- **Backend / Engine**: `C++17` & `MuPDF` (High-fidelity PDF parsing and byte-stream rendering).
- **Web Server**: `cpp-httplib` (Lightweight, header-only HTTP library for C++).
- **Data Serialization**: `nlohmann-json` (C++ JSON serialization).
- **Frontend**: Vanilla HTML5, CSS3, JavaScript.
- **Infrastructure**: Docker & Docker Compose.

## Quick Start & Installation

Requirements: You only need **Docker** and **Docker Compose** installed.

1. **Clone the repository:**
   ```bash
   git clone https://github.com/DMANDAVIYA/Custom-CPP-Text-Editor-and-Converter.git
   cd Custom-CPP-Text-Editor-and-Converter
   ```

2. **Launch the Engine:**
   Use Docker Compose to build the C++ server and spin up the frontend.
   ```bash
   docker compose up --build
   ```

3. **Access the Terminal:**
   Open your browser and navigate to:
   ```
   http://localhost:8000
   ```

## Architecture Overview

The system operates using a **"Canvas Projection"** paradigm:
1. **Extraction Pipeline**: When a PDF is uploaded, the C++ `PdfEngine` splits the PDF into high-resolution background imagery and extracts the microscopic DOM coordinate mappings (X, Y, Width, Height) of every single text block using MuPDF.
2. **Frontend Matrix Mapping**: The Javascript dynamically rescales the text coordinates perfectly over the rendered canvas image.
3. **Mutation Pipeline**: Upon clicking **COMPILE**, the `PdfMutator` intercepts modified DOM arrays from the Javascript payload. It mathematically expands the text bounding box, samples the exact pixel color from the background image matrix, injects a camouflage eraser stream directly into the PDF object array, and writes the new override text seamlessly inside the PDF payload map. 

## Roadmap

- **Phase 1-3:** System architecture, MuPDF graphical engines, and Retro Web UI (Completed ).
- **Phase 4:** Core Mutator overriding Engine (Completed ).
- **Phase 5 (Upcoming):** Multi-line Text Wrapping algorithms and Dynamic Font scaling engine.
- **Phase 6 (Upcoming):** Deep integration of Tesseract C++ API for raw image OCR parsing mode.

.
