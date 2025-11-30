#!/bin/bash
# Card Fifty-Two - Web Build Helper Script
# Automatically sources Emscripten environment and builds web version

set -e  # Exit on error

echo "================================"
echo "Card Fifty-Two - Web Build"
echo "================================"

# Source Emscripten SDK
if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
    echo "✓ Sourcing Emscripten SDK..."
    source "$HOME/emsdk/emsdk_env.sh"
else
    echo "✗ ERROR: Emscripten SDK not found at ~/emsdk/emsdk_env.sh"
    echo "Install with:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git ~/emsdk"
    echo "  cd ~/emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    exit 1
fi

# Verify emcc is available
if ! command -v emcc &> /dev/null; then
    echo "✗ ERROR: emcc not found in PATH after sourcing emsdk_env.sh"
    exit 1
fi

echo "✓ emcc version: $(emcc --version | head -1)"

# Build web version
echo ""
echo "Building web version..."
make web

echo ""
echo "================================"
echo "✅ Web build complete!"
echo "================================"
echo ""
echo "To test in browser:"
echo "  ./serve_web.sh"
echo "  # Then open: http://localhost:8000/index.html"
echo ""
