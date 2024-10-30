/**
 * "Hello, Triangle!" fragment shader.
 * Copyright (C) 2024  dbstream
 */
layout (location = 0) in struct {
	vec4 color;
} In;

layout (location = 0) out vec4 color;

layout (set = 0, binding = 0) uniform Uniform_T {
	float modulator;
} Uniform;

vec3
eotf (vec3 c)
{
	bvec3 cutoff = lessThan (c, vec3 (0.0031308));
	vec3 low = vec3 (12.92) * c;
	vec3 high = vec3 (1.055) * pow (c, vec3 (1.0 / 2.4)) - vec3 (0.055);
	return mix (high, low, cutoff);
}

void
main (void)
{
	color = vec4 (eotf (Uniform.modulator * In.color.rgb), In.color.a);
}
