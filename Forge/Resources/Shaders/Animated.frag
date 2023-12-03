#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec4 OutFragColor;

layout(location = 0) in vec2 InTexCoord;

layout(set = 0, binding = 1) uniform sampler2D u_Albedo;

void main()
{
	OutFragColor = texture(u_Albedo, InTexCoord);
}