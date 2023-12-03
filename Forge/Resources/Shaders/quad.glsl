#version 460

#extension GL_KHR_vulkan_glsl : enable

// Drawing clipped to quad triangle
// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/

layout(location = 0) out vec2 o_UV;

void main()
{
	o_UV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(o_UV * 2.0f - 1.0f, 0.0f, 1.0f);

	o_UV.y *= -1; // Because Sascha used CW order, but I use CCW
}