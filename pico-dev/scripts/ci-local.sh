#!/bin/bash
# Local CI/CD simulation script

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Pico CI/CD Pipeline (Local) ===${NC}"
echo "Running build and test pipeline..."
echo

# Stage 1: Environment check
echo -e "${YELLOW}[Stage 1/4] Environment Check${NC}"
if command -v docker &> /dev/null; then
    echo "✓ Docker is installed"
else
    echo -e "${RED}✗ Docker is not installed${NC}"
    exit 1
fi

if command -v docker-compose &> /dev/null; then
    echo "✓ Docker Compose is installed"
else
    echo -e "${RED}✗ Docker Compose is not installed${NC}"
    exit 1
fi

# Stage 2: Build firmware
echo -e "\n${YELLOW}[Stage 2/4] Building Firmware${NC}"
docker-compose run --rm pico-build

# Check if build succeeded
if [ -f "build/examples/blinky/blinky.elf" ]; then
    echo -e "${GREEN}✓ Build successful${NC}"
    ls -lh build/examples/blinky/blinky.*
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

# Stage 3: Test in QEMU
echo -e "\n${YELLOW}[Stage 3/4] Testing in QEMU${NC}"
docker-compose run --rm pico-test | tee test-output.log

# Stage 4: Verify results
echo -e "\n${YELLOW}[Stage 4/4] Verifying Results${NC}"

TESTS_PASSED=0
TESTS_FAILED=0

# Check for test header
if grep -q "Raspberry Pi Pico Blinky Test" test-output.log; then
    echo "✓ Test initialization detected"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗ Test initialization not found${NC}"
    ((TESTS_FAILED++))
fi

# Check for LED activity
if grep -q "LED ON" test-output.log && grep -q "LED OFF" test-output.log; then
    echo "✓ LED toggle activity detected"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗ LED activity not detected${NC}"
    ((TESTS_FAILED++))
fi

# Check for completion
if grep -q "Status: PASS" test-output.log; then
    echo "✓ Test completed successfully"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗ Test did not complete${NC}"
    ((TESTS_FAILED++))
fi

# Final report
echo -e "\n${BLUE}=== CI/CD Pipeline Results ===${NC}"
echo "Tests passed: $TESTS_PASSED"
echo "Tests failed: $TESTS_FAILED"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All stages passed!${NC}"
    echo -e "${GREEN}✓ Firmware is ready for deployment${NC}"
    exit 0
else
    echo -e "${RED}✗ Pipeline failed!${NC}"
    exit 1
fi