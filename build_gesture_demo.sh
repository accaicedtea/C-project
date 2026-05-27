#!/bin/bash
# Build and run the GTK4 Gesture Demo
# Usage: ./build_gesture_demo.sh

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "🎯 Building GTK4 Gesture Demo..."

# Compile with all necessary flags
gcc -Wall -Wextra -Werror -O2 -std=c11 \
  $(pkg-config --cflags gtk4 glib-2.0) \
  -I./include \
  -c examples/gesture_demo.c -o build/gesture_demo.o

# Link with all gesture object files
gcc -o build/gesture_demo \
  build/gesture_demo.o \
  build/utils/gesture_manager.o \
  $(pkg-config --libs gtk4 glib-2.0)

echo "✅ Build successful: build/gesture_demo"
echo ""
echo "🚀 Running demo..."
echo ""

# Run the demo
./build/gesture_demo
