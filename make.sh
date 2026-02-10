#!/bin/bash
set -e

# Configuration
CC="${CC:-cc}"
CFLAGS="${CFLAGS:--O2 -Wall}"
LDFLAGS="-lSDL2 -lm"
BUILD_DIR="build"

# Targets
TARGETS=(
    amazing_shader_window
    image_gen_window
    simple_animation_window
    wave_window
)

# Create build directory
mkdir -p "$BUILD_DIR"

# Clean old binaries
clean() {
    echo "Cleaning..."
    rm -f "${TARGETS[@]/#/$BUILD_DIR/}"
}

# Build all targets
build() {
    for target in "${TARGETS[@]}"; do
        echo "Building $target..."
        $CC $CFLAGS -o "$BUILD_DIR/$target" "test/${target}.c" $LDFLAGS
    done
    echo "Build complete. Binaries in $BUILD_DIR/"
}

# Show usage
usage() {
    echo "Usage: $0 [clean|build|help]"
    echo "  clean  - Remove built binaries"
    echo "  build  - Build all targets (default)"
    echo "  help   - Show this message"
}

case "${1:-build}" in
    clean) clean ;;
    build) build ;;
    help)  usage ;;
    *)     usage; exit 1 ;;
esac