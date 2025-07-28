# Raspberry Pi Pico Development & CI/CD Guide

This guide explains the complete Pico development environment with QEMU-based CI/CD testing.

## Overview

We've created a professional development environment that includes:
- **Automated builds** using Pico SDK and CMake
- **Containerized testing** with QEMU emulation
- **CI/CD pipeline** for automatic validation
- **Local development** tools and scripts

## Architecture

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Source Code   │────▶│     Build       │────▶│   Test (QEMU)   │
│  (blinky.c)     │     │  (Pico SDK)     │     │   Validation    │
└─────────────────┘     └─────────────────┘     └─────────────────┘
         │                       │                        │
         └───────────────────────┴────────────────────────┘
                                 │
                           ┌─────▼─────┐
                           │  CI/CD    │
                           │ Pipeline  │
                           └───────────┘
```

## Quick Demo

### 1. Run Complete CI/CD Pipeline Locally

```bash
cd pico-dev
./scripts/ci-local.sh
```

This will:
- Build the firmware in Docker
- Run it in QEMU
- Verify the output
- Report pass/fail status

### 2. Manual Build and Test

```bash
# Build
docker-compose run --rm pico-build

# Test
docker-compose run --rm pico-test
```

### 3. Development Workflow

```bash
# Edit the code
vim examples/blinky/blinky.c

# Build and test
./scripts/build.sh
./scripts/test.sh
```

## The Blinky Example

The `blinky.c` example demonstrates:
- GPIO control (LED on pin 25)
- UART output for debugging
- Timed test execution
- Automated verification

Key features for testing:
```c
// Defined test duration
#define TEST_DURATION_SEC 10

// Status reporting
printf("Status: PASS\n");

// QEMU-friendly exit
#ifdef QEMU_TEST
    return 0;
#endif
```

## CI/CD Pipeline (GitHub Actions)

The workflow (`.github/workflows/pico-firmware-ci.yml`):

1. **Build Stage**
   - Installs ARM toolchain
   - Clones Pico SDK
   - Builds firmware
   - Uploads artifacts

2. **Test Stage**
   - Downloads firmware
   - Runs in QEMU container
   - Captures output
   - Verifies behavior

3. **Analysis Stage**
   - Checks binary size
   - Generates reports
   - Validates constraints

## Docker Services

### pico-build
Compiles Pico firmware with SDK:
```bash
docker-compose run --rm pico-build
```

### pico-test
Runs firmware in QEMU:
```bash
docker-compose run --rm pico-test
```

### pico-dev
Interactive development:
```bash
docker-compose run --rm pico-dev
# Now in container shell
cd build && cmake .. && make
```

### pico-ci
Combined build for CI:
```bash
docker-compose run --rm pico-ci
```

## Writing Testable Firmware

Best practices for QEMU testing:

1. **Use UART for output**
   ```c
   stdio_init_all();
   printf("Test message\n");
   ```

2. **Add status markers**
   ```c
   printf("Status: PASS\n");
   printf("Status: FAIL - reason\n");
   ```

3. **Time-bound execution**
   ```c
   while (time_elapsed < TEST_DURATION) {
       // Test code
   }
   ```

4. **Clean exit for QEMU**
   ```c
   #ifdef QEMU_TEST
   return 0;  // Exit cleanly
   #endif
   ```

## Test Verification

The CI/CD verifies:
- Initialization messages appear
- Expected behavior occurs (LED ON/OFF)
- Test completes within timeout
- Status is PASS

Example verification:
```bash
if grep -q "Status: PASS" test-output.log; then
    echo "Test passed!"
fi
```

## Adding New Firmware

1. Create new example:
   ```bash
   mkdir -p examples/myapp
   ```

2. Add CMakeLists.txt:
   ```cmake
   add_executable(myapp myapp.c)
   target_link_libraries(myapp pico_stdlib)
   pico_add_extra_outputs(myapp)
   ```

3. Update main CMakeLists.txt:
   ```cmake
   add_subdirectory(examples/myapp)
   ```

4. Write testable code with status output

## Local vs CI Environment

### Local Development
- Fast iteration
- Direct hardware access
- Debugging tools

### CI/CD Environment
- Automated validation
- Consistent environment
- No hardware needed

## Debugging Failed Tests

1. **Check build logs**
   ```bash
   docker-compose logs pico-build
   ```

2. **View test output**
   ```bash
   cat test-output.log
   ```

3. **Run interactively**
   ```bash
   docker-compose run --rm pico-test bash
   # Manually run QEMU commands
   ```

4. **Enable debug output**
   ```c
   #ifdef DEBUG
   printf("Debug: value = %d\n", value);
   #endif
   ```

## Performance Monitoring

The CI/CD reports:
- Binary size
- Memory usage
- Build time
- Test duration

## Integration with Hardware

While QEMU tests basic functionality, hardware testing adds:
- Real timing verification
- Peripheral interaction
- Power consumption
- Physical I/O

## Best Practices

1. **Keep tests short** (<30 seconds)
2. **Use clear status messages**
3. **Test one feature at a time**
4. **Document expected output**
5. **Version control test data**

## Troubleshooting

### Build Fails
- Check CMake syntax
- Verify SDK path
- Review compiler errors

### Test Timeout
- Reduce test duration
- Check for infinite loops
- Verify UART initialization

### No Output
- Enable stdio_uart in CMake
- Add delay after init
- Check QEMU serial settings

## Future Enhancements

- Hardware-in-the-loop testing
- Performance benchmarking
- Code coverage reports
- Multi-board support
- Integration tests

This environment provides a complete solution for Pico development with automated testing, making it easy to develop and validate firmware without physical hardware.