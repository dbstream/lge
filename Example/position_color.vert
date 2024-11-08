/**
 * "Hello, Triangle!" vertex shader.
 * Copyright (C) 2024  dbstream
 */
layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;

layout (location = 0) out struct {
	vec4 color;
} Out;

layout (set = 0, binding = 0) uniform Uniform_T {
	mat4 view_perspective;
} Uniform;

layout (push_constant) uniform Push_T {
	mat4 model;
} Push;

void
main (void)
{
	gl_Position = Uniform.view_perspective * Push.model * position;
	Out.color = color;
}
