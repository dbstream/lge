#!/bin/sh

GLSLFLAGS="-V100 --glsl-version 460 -Os -g0"

set -e
glslang $GLSLFLAGS -o debugui.frag.txt --vn debugui_frag debugui.frag 
glslang $GLSLFLAGS -o debugui.vert.txt --vn debugui_vert debugui.vert
