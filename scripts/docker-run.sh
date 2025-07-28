#!/bin/bash
# Helper script for running QEMU RP2040 in Docker

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
IMAGE_NAME="murr2k/qemu-rp2040:latest"
CONTAINER_NAME="qemu-rp2040-session"
WORKSPACE_DIR="${PWD}/workspace"

# Function to display usage
usage() {
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo ""
    echo "Commands:"
    echo "  build              Build the Docker image"
    echo "  run PROGRAM        Run a program in QEMU"
    echo "  debug PROGRAM      Run with GDB server enabled"
    echo "  shell              Start interactive shell"
    echo "  test               Run all test programs"
    echo "  clean              Remove containers and images"
    echo ""
    echo "Options:"
    echo "  -h, --help         Show this help message"
    echo "  -v, --verbose      Enable verbose output"
    echo ""
    echo "Examples:"
    echo "  $0 build"
    echo "  $0 run tests/rp2040/test_uart.elf"
    echo "  $0 debug tests/rp2040/test_gpio.elf"
    echo "  $0 shell"
}

# Build Docker image
build_image() {
    echo -e "${GREEN}Building QEMU RP2040 Docker image...${NC}"
    docker build -t ${IMAGE_NAME} .
    echo -e "${GREEN}Build complete!${NC}"
}

# Run program in QEMU
run_program() {
    local program=$1
    if [ -z "$program" ]; then
        echo -e "${RED}Error: No program specified${NC}"
        usage
        exit 1
    fi
    
    echo -e "${GREEN}Running $program in QEMU...${NC}"
    docker run --rm -it \
        -v "${WORKSPACE_DIR}:/workspace" \
        --name ${CONTAINER_NAME} \
        ${IMAGE_NAME} \
        qemu-system-arm -machine raspberrypi-pico \
        -kernel "/workspace/$program" \
        -serial stdio \
        -monitor none \
        -nographic
}

# Debug program with GDB
debug_program() {
    local program=$1
    if [ -z "$program" ]; then
        echo -e "${RED}Error: No program specified${NC}"
        usage
        exit 1
    fi
    
    echo -e "${YELLOW}Starting QEMU with GDB server on port 1234...${NC}"
    echo -e "${YELLOW}Connect with: arm-none-eabi-gdb -ex 'target remote :1234'${NC}"
    
    docker run --rm -it \
        -v "${WORKSPACE_DIR}:/workspace" \
        -p 1234:1234 \
        --name ${CONTAINER_NAME} \
        ${IMAGE_NAME} \
        qemu-system-arm -machine raspberrypi-pico \
        -kernel "/workspace/$program" \
        -serial stdio \
        -s -S
}

# Start interactive shell
start_shell() {
    echo -e "${GREEN}Starting interactive shell...${NC}"
    docker run --rm -it \
        -v "${WORKSPACE_DIR}:/workspace" \
        --name ${CONTAINER_NAME} \
        ${IMAGE_NAME} \
        /bin/bash
}

# Run all tests
run_tests() {
    echo -e "${GREEN}Running all test programs...${NC}"
    
    # Build tests first
    docker run --rm \
        -v "${WORKSPACE_DIR}:/workspace" \
        ${IMAGE_NAME} \
        bash -c "cd /workspace/tests/rp2040 && make all"
    
    # Run each test
    for test in test_uart test_gpio test_timer; do
        echo -e "\n${YELLOW}Running $test...${NC}"
        docker run --rm \
            -v "${WORKSPACE_DIR}:/workspace" \
            ${IMAGE_NAME} \
            timeout 10 qemu-system-arm -machine raspberrypi-pico \
            -kernel "/workspace/tests/rp2040/${test}.elf" \
            -serial stdio \
            -monitor none \
            -nographic || true
        echo -e "${GREEN}$test complete${NC}"
    done
}

# Clean up Docker resources
clean_docker() {
    echo -e "${YELLOW}Cleaning up Docker resources...${NC}"
    
    # Stop and remove containers
    docker ps -a | grep ${CONTAINER_NAME} | awk '{print $1}' | xargs -r docker rm -f
    
    # Remove image
    docker rmi ${IMAGE_NAME} || true
    
    echo -e "${GREEN}Cleanup complete${NC}"
}

# Create workspace directory if it doesn't exist
mkdir -p "${WORKSPACE_DIR}"

# Main script logic
case "$1" in
    build)
        build_image
        ;;
    run)
        shift
        run_program "$@"
        ;;
    debug)
        shift
        debug_program "$@"
        ;;
    shell)
        start_shell
        ;;
    test)
        run_tests
        ;;
    clean)
        clean_docker
        ;;
    -h|--help|"")
        usage
        ;;
    *)
        echo -e "${RED}Error: Unknown command '$1'${NC}"
        usage
        exit 1
        ;;
esac