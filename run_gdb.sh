#!/bin/sh

echo Starting GDB
cd EditorLayer # Goto EditorLayer

# Initialize GDB
XCURSOR_THEME=Breeze_Light XCURSOR_SIZE=24 gdb ../cmake-build-debug/EditorLayer/EditorLayer
