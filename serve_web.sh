#!/bin/bash
# Card Fifty-Two - Web Server Helper
# Serves the web build on localhost:8000

cd index 2>/dev/null || {
    echo "✗ ERROR: index/ directory not found"
    echo "Run ./build_web.sh first to create web build"
    exit 1
}

echo "================================"
echo "Card Fifty-Two - Web Server"
echo "================================"
echo ""
echo "Starting HTTP server..."
echo "URL: http://localhost:8000/index.html"
echo ""
echo "Press Ctrl+C to stop server"
echo ""

python3 -m http.server 8000
