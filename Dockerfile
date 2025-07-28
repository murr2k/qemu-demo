# Multi-stage Dockerfile for QEMU RP2040 emulation
# Stage 1: Build QEMU with RP2040 support
FROM ubuntu:22.04 AS qemu-builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    python3-pip \
    ninja-build \
    pkg-config \
    libglib2.0-dev \
    libpixman-1-dev \
    git \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Install meson
RUN pip3 install meson

# Clone QEMU source (would be replaced with actual QEMU + patches)
WORKDIR /build
RUN git clone --depth 1 --branch v8.2.0 https://gitlab.com/qemu-project/qemu.git

# Copy RP2040 implementation files
COPY hw/ /build/qemu/hw/
COPY include/ /build/qemu/include/

# Build QEMU
WORKDIR /build/qemu
RUN mkdir build && cd build && \
    ../configure --target-list=arm-softmmu \
    --enable-debug \
    --disable-werror \
    --prefix=/opt/qemu && \
    ninja && \
    ninja install

# Stage 2: Runtime image with ARM toolchain
FROM ubuntu:22.04

# Install runtime dependencies and ARM toolchain
RUN apt-get update && apt-get install -y \
    libglib2.0-0 \
    libpixman-1-0 \
    gcc-arm-none-eabi \
    gdb-multiarch \
    make \
    python3 \
    python3-pip \
    vim \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Copy QEMU from builder
COPY --from=qemu-builder /opt/qemu /opt/qemu

# Add QEMU to PATH
ENV PATH="/opt/qemu/bin:${PATH}"

# Create workspace directory
WORKDIR /workspace

# Copy test programs and documentation
COPY tests/ /workspace/tests/
COPY README.md QUICKSTART.md /workspace/

# Default command shows help
CMD ["qemu-system-arm", "-machine", "help"]