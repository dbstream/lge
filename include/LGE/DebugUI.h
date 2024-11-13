/**
 * DebugUI - drawing debug information to the screen
 * Copyright (C) 2024  dbstream
 */
#pragma once

#include <LGE/Vulkan.h>

namespace LGE {

enum class DebugUICorner {
	TOP_LEFT,
	TOP_RIGHT,
	BOTTOM_LEFT,
	BOTTOM_RIGHT
};

void
DebugUIInit (void);

void
DebugUITerminate (void);

/** 
 * Draw text to the Debug UI.
 *
 * @param text text to draw
 * @param x, y position of text
 * @param corner which corner the position is relative to
 * @param r, g, b, a color and transparency of text
 */
void
DebugUIDrawText (const char *text, int x, int y, DebugUICorner corner,
	float r, float g, float b, float a);

/**
 * Draw text to the Debug UI using a printf format specifier.
 *
 * @param fmt printf format
 * @param x, y position of text
 * @param corner which corner the position is relative to
 * @param r, g, b, a color and transparency of text
 */
void
DebugUIPrintf (int x, int y, DebugUICorner corner,
	float r, float g, float b, float a,
	const char *fmt, ...);

void
DebugUIDraw (VkCommandBuffer cmd);

}
