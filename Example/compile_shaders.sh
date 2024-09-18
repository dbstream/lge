#!/bin/sh

GLSLFLAGS="-V100 --glsl-version 460 -Os -g0"

set -e
glslang $GLSLFLAGS -o position_color.frag.txt --vn fragment_shader position_color.frag 
glslang $GLSLFLAGS -o position_color.vert.txt --vn vertex_shader position_color.vert
