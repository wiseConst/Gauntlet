#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec4 out_Position;
layout(location = 2) out vec4 out_Albedo;

layout(location = 0) in vec3 in_Color;
layout(location = 1) in vec3 in_FragPos;

void main()
{
	out_Position = vec4(in_FragPos, 1.0f);
	out_Albedo = vec4(in_Color, 1.0f);
}