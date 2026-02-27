FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    libmupdf-dev \
    nlohmann-json3-dev \
    libssl-dev \
    zlib1g-dev \
    libfreetype6-dev \
    libjpeg-dev \
    libharfbuzz-dev \
    libjbig2dec0-dev \
    libopenjp2-7-dev \
    libmujs-dev \
    libgumbo-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app

COPY . /usr/src/app

# Default command to build and run the server natively
CMD mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc) && ./pdfmaker_engine
