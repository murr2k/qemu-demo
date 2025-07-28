# Dockerfile for Pico SDK development with QEMU testing
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
    build-essential \
    git \
    python3 \
    python3-pip \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /workspace

# Clone Pico SDK
RUN git clone --depth 1 --branch 1.5.1 https://github.com/raspberrypi/pico-sdk.git

# Set SDK path
ENV PICO_SDK_PATH=/workspace/pico-sdk

# Initialize SDK submodules
RUN cd ${PICO_SDK_PATH} && \
    git submodule update --init

# Copy project files
COPY . /workspace/pico-dev/

# Create build directory
RUN mkdir -p /workspace/pico-dev/build

WORKDIR /workspace/pico-dev

# Default command builds the project
CMD ["bash", "-c", "cd build && cmake .. && make -j$(nproc)"]