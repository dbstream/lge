/**
 * Rasterize DebugUI text.
 * Copyright (C) 2024  dbstream
 */
layout (location = 0) in struct {
	vec2 uv;
	vec4 modulator;
} In;

layout (location = 0) out vec4 color;

layout (set = 0, binding = 0) uniform sampler2D font_bitmap;

void
main (void)
{
	color = vec4 (In.modulator.rgb, In.modulator.a * texture(font_bitmap, In.uv).r);
}
