#!/bin/bash
# =============================================================
#  EyeCare v6.4 Build Script (Linux Cross-Compile with llvm-mingw)
#
#  Prerequisites:
#    - llvm-mingw (or MinGW-w64) toolchain installed
#    - x86_64-w64-mingw32-clang++ in PATH
#
#  Output: EyeCare.exe
#
#  Note: MinGW builds may trigger antivirus false positives.
#  For lowest false positive rate, use MSVC build (build.bat).
# =============================================================

set -e

echo "============================================"
echo "  EyeCare v6.4 - Cross-Compiling..."
echo "============================================"

# Compile resource script
echo "[1/2] Compiling resources..."
if command -v x86_64-w64-mingw32-windres &> /dev/null; then
    x86_64-w64-mingw32-windres EyeCare.rc -O coff -o EyeCare.res
    RES=EyeCare.res
    echo "  Resources compiled successfully."
else
    echo "  windres not found, building without resources."
    RES=""
fi

# Compile and link
echo "[2/2] Compiling main program..."
x86_64-w64-mingw32-clang++ -static -mwindows -municode -O2 -std=c++17 \
    -DUNICODE -D_UNICODE \
    -o EyeCare.exe \
    EyeCare.cpp $RES \
    -lgdiplus -lole32 -lshell32 -lcomctl32 -lwinmm -ladvapi32 -lshfolder

if [ -f EyeCare.exe ]; then
    echo ""
    echo "============================================"
    echo "  Build successful!"
    echo "============================================"
    ls -lh EyeCare.exe
    # Clean up
    rm -f EyeCare.res EyeCare.obj 2>/dev/null
else
    echo ""
    echo "============================================"
    echo "  Build failed!"
    echo "============================================"
    exit 1
fi
