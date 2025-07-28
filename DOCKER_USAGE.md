# Docker Usage Guide for QEMU RP2040

This guide explains how to use the Docker container for QEMU RP2040 emulation.

## Quick Start

### 1. Build the Docker Image

```bash
./scripts/docker-run.sh build
```

Or using Docker Compose:
```bash
docker-compose build
```

### 2. Run a Test Program

```bash
# Run UART test
./scripts/docker-run.sh run tests/rp2040/test_uart.elf

# Or using docker-compose
docker-compose run --rm qemu-rp2040
```

### 3. Interactive Development

Start a shell inside the container:
```bash
./scripts/docker-run.sh shell
```

## Docker Image Contents

The Docker image includes:
- QEMU with RP2040 support
- ARM GCC toolchain (arm-none-eabi-gcc)
- GDB for ARM (gdb-multiarch)
- Build tools (make, ninja)
- Test programs and examples

## Usage Scenarios

### Running Your Own Program

1. Place your program in the `workspace/` directory
2. Run it with:
```bash
./scripts/docker-run.sh run your-program.elf
```

### Debugging with GDB

1. Start QEMU with GDB server:
```bash
./scripts/docker-run.sh debug tests/rp2040/test_gpio.elf
```

2. In another terminal, connect with GDB:
```bash
docker exec -it qemu-rp2040-session gdb-multiarch
(gdb) target remote :1234
(gdb) continue
```

### Building Programs Inside Container

```bash
# Start shell
./scripts/docker-run.sh shell

# Inside container
cd /workspace/tests/rp2040
make all
```

### Running All Tests

```bash
./scripts/docker-run.sh test
```

## Docker Compose Services

The `docker-compose.yml` provides several services:

- `qemu-rp2040`: Default service, runs UART test
- `qemu-rp2040-dev`: Development shell
- `qemu-rp2040-debug`: GDB debugging setup
- `qemu-rp2040-vnc`: VNC display (future feature)

### Examples:

```bash
# Development shell
docker-compose run --rm qemu-rp2040-dev

# Debugging session
docker-compose run --rm -p 1234:1234 qemu-rp2040-debug

# Custom command
docker-compose run --rm qemu-rp2040 qemu-system-arm -machine help
```

## Volume Mounts

The container mounts:
- `./workspace:/workspace` - Your working directory
- `./tests:/workspace/tests:ro` - Test programs (read-only)

Place your RP2040 programs in the `workspace/` directory to access them in the container.

## Environment Variables

You can customize behavior with environment variables:

```bash
# Custom QEMU options
docker run -e QEMU_OPTS="-d guest_errors" murr2k/qemu-rp2040 ...

# Different machine options
docker run -e MACHINE_OPTS="-machine raspberrypi-pico,firmware=boot.bin" ...
```

## Networking

By default, the container uses host networking for serial console access. For isolated networking:

```bash
docker run --network bridge -p 5555:5555 murr2k/qemu-rp2040 ...
```

## Troubleshooting

### Permission Issues
If you get permission errors:
```bash
docker run --user $(id -u):$(id -g) murr2k/qemu-rp2040 ...
```

### Can't Connect to GDB
Ensure port 1234 is available:
```bash
lsof -i :1234
```

### Container Exits Immediately
Add `-it` flags for interactive mode:
```bash
docker run -it murr2k/qemu-rp2040 /bin/bash
```

## Advanced Usage

### Multi-Architecture Builds

Build for multiple platforms:
```bash
docker buildx build --platform linux/amd64,linux/arm64 -t murr2k/qemu-rp2040 .
```

### Custom QEMU Build

Modify the Dockerfile to use your QEMU fork:
```dockerfile
# Replace the git clone line with:
RUN git clone --depth 1 https://github.com/yourusername/qemu.git
```

### CI/CD Integration

Use in GitHub Actions:
```yaml
- name: Run RP2040 Tests
  run: |
    docker run --rm \
      -v ${{ github.workspace }}:/workspace \
      murr2k/qemu-rp2040 \
      /workspace/scripts/run-tests.sh
```

## Performance Tips

1. Use `--rm` to automatically remove containers
2. Mount only necessary directories
3. Use specific tags instead of `:latest`
4. Enable BuildKit for faster builds:
   ```bash
   DOCKER_BUILDKIT=1 docker build .
   ```

## Security Considerations

- The container runs with minimal privileges
- GDB debugging requires `SYS_PTRACE` capability
- Use read-only mounts where possible
- Don't expose unnecessary ports

## Getting Help

```bash
# Show available commands
./scripts/docker-run.sh --help

# QEMU help
docker run --rm murr2k/qemu-rp2040 qemu-system-arm --help

# Machine-specific help
docker run --rm murr2k/qemu-rp2040 qemu-system-arm -machine raspberrypi-pico,help
```