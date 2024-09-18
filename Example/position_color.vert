/**
 * "Hello, Triangle!" vertex shader.
 * Copyright (C) 2024  dbstream
 */
layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;

layout (location = 0) out struct {
	vec4 color;
} Out;

void
main (void)
{
	gl_Position = position;
	Out.color = color;
}
