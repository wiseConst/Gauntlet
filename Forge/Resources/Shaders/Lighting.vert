#version 460

// Drawing clipped to quad triangle
// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/

layout(location = 0) out vec2 OutTexCoord;

void main()
{
	OutTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(OutTexCoord * 2.0f - 1.0f, 0.0f, 1.0f);

	OutTexCoord.y *= -1.0f; // Because Sascha used CW order, but I use CCW
}