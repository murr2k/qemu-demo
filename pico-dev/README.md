# Raspberry Pi Pico Development Environment

Complete development environment for Raspberry Pi Pico with automated CI/CD testing on QEMU.

## Features

- 🔧 **Pico SDK Integration** - Full SDK support with CMake build system
- 🐳 **Docker-based Development** - Consistent environment across platforms
- 🧪 **Automated Testing** - Run firmware in QEMU automatically
- 🚀 **CI/CD Pipeline** - GitHub Actions for build and test
- 📊 **Test Verification** - Automated output validation

## Quick Start

### 1. Build the Blinky Example

```bash
# Using Docker
docker-compose run --rm pico-build

# Or using local script
./scripts/build.sh
```

### 2. Test in QEMU

```bash
# Using Docker
docker-compose run --rm pico-test

# Or using local script
./scripts/test.sh
```

### 3. Run Full CI/CD Pipeline Locally

```bash
./scripts/ci-local.sh
```

## Project Structure

```
pico-dev/
├── CMakeLists.txt          # Main CMake configuration
├── examples/
│   └── blinky/            # LED blink example
│       ├── CMakeLists.txt
│       └── blinky.c
├── scripts/               # Automation scripts
│   ├── build.sh          # Build firmware
│   ├── test.sh           # Test in QEMU
│   └── ci-local.sh       # Run CI/CD locally
├── .github/
│   └── workflows/
│       └── build-and-test.yml  # GitHub Actions CI/CD
├── docker-compose.yml     # Docker services
└── Dockerfile            # Build environment
```

## Development Workflow

### Local Development

1. **Edit Code**: Modify `examples/blinky/blinky.c`
2. **Build**: `./scripts/build.sh`
3. **Test**: `./scripts/test.sh`
4. **Verify**: Check console output for test results

### Docker Development

```bash
# Interactive development shell
docker-compose run --rm pico-dev

# Inside container
cd build
cmake ..
make
```

### CI/CD Pipeline

The GitHub Actions workflow automatically:
1. Builds firmware for all examples
2. Runs tests in QEMU
3. Verifies output for correctness
4. Generates size reports
5. Archives artifacts

## Writing Tests

The blinky example demonstrates testable firmware:

```c
// Print status for automated verification
printf("Status: PASS\n");

// Use defined test duration
#define TEST_DURATION_SEC 10

// Exit cleanly for QEMU
#ifdef QEMU_TEST
    return 0;
#endif
```

## Docker Services

- `pico-build`: Compile firmware
- `pico-test`: Run in QEMU
- `pico-dev`: Interactive shell
- `pico-ci`: Combined build+test

## Adding New Examples

1. Create directory: `examples/yourapp/`
2. Add `CMakeLists.txt`:
   ```cmake
   add_executable(yourapp yourapp.c)
   target_link_libraries(yourapp pico_stdlib)
   pico_add_extra_outputs(yourapp)
   ```
3. Update main `CMakeLists.txt`:
   ```cmake
   add_subdirectory(examples/yourapp)
   ```

## Test Output Verification

Tests are verified by checking for:
- Initialization messages
- Expected behavior (LED ON/OFF)
- Completion status
- No timeout or crashes

## Debugging

### View Detailed Output
```bash
docker-compose run --rm pico-test
```

### Connect GDB
```bash
# Start QEMU with GDB server
docker run -p 1234:1234 murr2k/qemu-rp2040 \
  qemu-system-arm -machine raspberrypi-pico \
  -kernel firmware.elf -s -S

# Connect GDB
arm-none-eabi-gdb firmware.elf
(gdb) target remote :1234
```

## Environment Variables

- `PICO_SDK_PATH`: Path to Pico SDK (auto-downloaded if not set)
- `CMAKE_BUILD_TYPE`: Debug or Release (default: Release)

## Troubleshooting

### Build Fails
- Ensure Docker is running
- Check CMakeLists.txt syntax
- Verify SDK is downloaded

### Test Timeout
- Increase timeout in scripts
- Check for infinite loops
- Verify UART output is enabled

### No Output
- Ensure `stdio_init_all()` is called
- Check UART is enabled in CMake
- Verify serial settings

## Performance Metrics

Example output from CI/CD:
```
   text    data     bss     dec     hex filename
  11264    2260     264   13788    35dc blinky.elf
```

## Contributing

1. Fork the repository
2. Create feature branch
3. Add tests for new features
4. Ensure CI/CD passes
5. Submit pull request

## License

This project is part of the QEMU RP2040 demonstration and follows the same licensing.