/**
 * Rasterize DebugUI text.
 * Copyright (C) 2024  dbstream
 */
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 modulator;

layout (location = 0) out struct {
	vec2 uv;
	vec4 modulator;
} Out;

void
main (void)
{
	Out.uv = uv;
	Out.modulator = modulator;
	gl_Position = vec4 (vec2 (-1.0, -1.0) + 2.0 * position, 0.0, 1.0);
}
