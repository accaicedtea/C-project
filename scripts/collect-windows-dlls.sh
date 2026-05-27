#!/bin/bash
# Collect DLLs and resources for Windows cross-compilation
# Usage: bash scripts/collect-windows-dlls.sh <mingw_prefix> <build_dir>

MINGW_PREFIX=${1:-x86_64-w64-mingw32}
BUILD_DIR=${2:-build-x86_64}

echo "🔧 Collecting DLLs for $MINGW_PREFIX into $BUILD_DIR..."

# Create subdirectories
mkdir -p "$BUILD_DIR/lib"
mkdir -p "$BUILD_DIR/share"

# Find MINGW installation
MINGW_ROOT=$(${MINGW_PREFIX}-gcc -print-sysroot)

if [ -z "$MINGW_ROOT" ]; then
    echo "❌ Could not find MINGW root directory"
    exit 1
fi

echo "📁 MINGW root: $MINGW_ROOT"

# Copy essential GTK4 and GLib DLLs
GTK4_LIBS=(
    "libgtk-4.dll"
    "libgdk-4.dll"
    "libgio-2.0.dll"
    "libglib-2.0.dll"
    "libgobject-2.0.dll"
    "libgthread-2.0.dll"
    "libpangocairo-1.0.dll"
    "libpango-1.0.dll"
    "libharfbuzz.dll"
    "libcairo.dll"
    "libpixman-1.dll"
    "libpng16.dll"
    "libjpeg-9.dll"
    "libfreetype-6.dll"
    "libfribidi.dll"
    "libiconv-2.dll"
    "libintl-8.dll"
)

echo "📦 Copying DLLs..."
for lib in "${GTK4_LIBS[@]}"; do
    # Try both lib and bin directories
    if [ -f "$MINGW_ROOT/lib/$lib" ]; then
        cp "$MINGW_ROOT/lib/$lib" "$BUILD_DIR/" 2>/dev/null && echo "  ✓ $lib"
    elif [ -f "$MINGW_ROOT/bin/$lib" ]; then
        cp "$MINGW_ROOT/bin/$lib" "$BUILD_DIR/" 2>/dev/null && echo "  ✓ $lib"
    fi
done

# Copy GTK4 themes and schemas
if [ -d "$MINGW_ROOT/share/gtk-4.0" ]; then
    echo "📋 Copying GTK4 theme..."
    cp -r "$MINGW_ROOT/share/gtk-4.0" "$BUILD_DIR/share/"
fi

if [ -d "$MINGW_ROOT/share/glib-2.0/schemas" ]; then
    echo "📋 Copying GLib schemas..."
    mkdir -p "$BUILD_DIR/share/glib-2.0"
    cp -r "$MINGW_ROOT/share/glib-2.0/schemas" "$BUILD_DIR/share/glib-2.0/"
fi

# Copy icons and other resources
if [ -d "$MINGW_ROOT/share/icons" ]; then
    echo "🎨 Copying icons..."
    cp -r "$MINGW_ROOT/share/icons" "$BUILD_DIR/share/" 2>/dev/null || true
fi

echo "✅ Collection complete!"
ls -la "$BUILD_DIR"
