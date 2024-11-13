# GNUmakefile for building some tools.
# Copyright (C) 2024  dbstream

# --- Why on earth is this a GNUmakefile? ---
#
# Because it works on my machine, and that's what is needed for these
# tools, as they need to run very seldom (only for font baking at the
# time of writing).  2024-11-11

CXX := g++
CXXFLAGS := -O2 -std=c++20

CXXFLAGS-generate_font := $(shell pkg-config --cflags --libs freetype2)

all:

%: %.cc
	$(CXX) $(CXXFLAGS) -o $@ $< $(CXXFLAGS-$@)
