#!/bin/bash
# Build script for Pico development

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Check if SDK path is set
if [ -z "$PICO_SDK_PATH" ]; then
    echo -e "${YELLOW}PICO_SDK_PATH not set, using local pico-sdk${NC}"
    export PICO_SDK_PATH=$(pwd)/pico-sdk
fi

# Clone SDK if not present
if [ ! -d "$PICO_SDK_PATH" ]; then
    echo -e "${GREEN}Cloning Pico SDK...${NC}"
    git clone --depth 1 --branch 1.5.1 https://github.com/raspberrypi/pico-sdk.git $PICO_SDK_PATH
    cd $PICO_SDK_PATH
    git submodule update --init
    cd -
fi

# Create build directory
mkdir -p build
cd build

# Configure
echo -e "${GREEN}Configuring build...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo -e "${GREEN}Building firmware...${NC}"
make -j$(nproc)

# Show output files
echo -e "${GREEN}Build complete! Output files:${NC}"
find . -name "*.elf" -o -name "*.uf2" -o -name "*.hex" | sort

echo -e "${GREEN}Done!${NC}"