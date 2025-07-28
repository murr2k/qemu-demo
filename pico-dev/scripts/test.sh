#!/bin/bash
# Test script for running firmware in QEMU

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Default firmware
FIRMWARE=${1:-"build/examples/blinky/blinky.elf"}

# Check if firmware exists
if [ ! -f "$FIRMWARE" ]; then
    echo -e "${RED}Error: Firmware not found: $FIRMWARE${NC}"
    echo "Please build first with: ./scripts/build.sh"
    exit 1
fi

echo -e "${GREEN}Testing firmware: $FIRMWARE${NC}"

# Check if running in Docker
if [ -f /.dockerenv ]; then
    # Inside Docker, QEMU should be available
    QEMU_CMD="qemu-system-arm"
else
    # Outside Docker, use our Docker image
    echo -e "${YELLOW}Running via Docker...${NC}"
    docker run --rm -v $(pwd):/workspace \
        murr2k/qemu-rp2040:latest \
        timeout 15 qemu-system-arm \
        -machine raspberrypi-pico \
        -kernel /workspace/$FIRMWARE \
        -serial stdio \
        -monitor none \
        -nographic
    exit $?
fi

# Run test with timeout
echo -e "${YELLOW}Starting QEMU (15 second timeout)...${NC}"
echo "======================================"

timeout 15 $QEMU_CMD \
    -machine raspberrypi-pico \
    -kernel $FIRMWARE \
    -serial stdio \
    -monitor none \
    -nographic | tee test-output.log

# Check test results
echo "======================================"
echo -e "${GREEN}Checking test results...${NC}"

if grep -q "Status: PASS" test-output.log; then
    echo -e "${GREEN}✓ Test PASSED!${NC}"
    exit 0
else
    echo -e "${RED}✗ Test FAILED!${NC}"
    exit 1
fi