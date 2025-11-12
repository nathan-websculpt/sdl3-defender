#!/usr/bin/env bash
set -e

PROJECT_NAME="SDL3Defender"
BUILD_DIR="build"
BIN_DIR="$BUILD_DIR/bin"
EXECUTABLE="$BIN_DIR/$PROJECT_NAME"

# Detect shell
PARENT_SHELL=$(ps -p $PPID -o comm= | tr -d ' ' 2>/dev/null || echo "unknown")
echo "Detected parent shell: $PARENT_SHELL"

# Clean way to add paths without duplicates
echo "Setting SDL3 environment variables directly"

# For LD_LIBRARY_PATH
if [[ ":$LD_LIBRARY_PATH:" != *":$HOME/.local/lib:"* ]]; then
    export LD_LIBRARY_PATH="$HOME/.local/lib:${LD_LIBRARY_PATH:-}"
fi

# For PATH
if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
    export PATH="$HOME/.local/bin:${PATH:-}"
fi

# For PKG_CONFIG_PATH
if [[ ":$PKG_CONFIG_PATH:" != *":$HOME/.local/lib/pkgconfig:"* ]]; then
    export PKG_CONFIG_PATH="$HOME/.local/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
fi

# Set audio driver
export SDL_AUDIODRIVER=pipewire

echo "SDL3 library path: $LD_LIBRARY_PATH"
echo "Audio driver: $SDL_AUDIODRIVER"

# Detect number of CPU cores for parallel build
CORES=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

# 1. Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# 2. Configure with cmake if needed
if [ ! -f "$BUILD_DIR/Makefile" ] || [ "CMakeLists.txt" -nt "$BUILD_DIR/Makefile" ]; then
    echo "Running CMake configuration..."
    cmake -S . -B "$BUILD_DIR"
fi

# 3. Build project
echo "Building $PROJECT_NAME..."
cmake --build "$BUILD_DIR" -j"$CORES"

# 4. Run the game
if [ -f "$EXECUTABLE" ]; then
    echo ""
    echo "Launching $PROJECT_NAME..."
    echo "----------------------------------------------------"
    echo ""
    (cd "$BIN_DIR" && "./$PROJECT_NAME")
else
    echo "Build failed or executable not found at: $EXECUTABLE"
    exit 1
fi