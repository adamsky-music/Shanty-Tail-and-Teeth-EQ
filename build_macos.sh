#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

if [ ! -f "JUCE/CMakeLists.txt" ]; then
    echo "=== Cloning JUCE 9.0.6 ==="
    git clone --depth 1 --branch 9.0.6 https://github.com/juce-framework/JUCE.git JUCE
fi

ARCH="$(uname -m)"
echo "=== Configuring CMake for $ARCH ==="
cmake -B build-local -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="$ARCH"

echo "=== Building Shanty Tail & Teeth EQ (VST3, AU, Standalone) ==="
cmake --build build-local --config Release -j "$(sysctl -n hw.ncpu)"

echo "=== Installing to ~/Library/Audio/Plug-Ins ==="
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3" "$HOME/Library/Audio/Plug-Ins/Components"
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/Shanty Tail and Teeth EQ.vst3"
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/Shanty Tail and Teeth EQ.component"
cp -R "build-local/ShantyTailAndTeethEQ_artefacts/Release/VST3/Shanty Tail and Teeth EQ.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/"
cp -R "build-local/ShantyTailAndTeethEQ_artefacts/Release/AU/Shanty Tail and Teeth EQ.component" "$HOME/Library/Audio/Plug-Ins/Components/"

echo "=== Ad-hoc signing ==="
codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/VST3/Shanty Tail and Teeth EQ.vst3"
codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/Components/Shanty Tail and Teeth EQ.component"

echo "=== Done! Restart your DAW and scan plugins. ==="
