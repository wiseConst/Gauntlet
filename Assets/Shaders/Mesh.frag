#version 460

layout(location = 0) in vec4 InColor;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InTangent;
layout(location = 4) in vec3 InBitangent;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D Diffuse;
layout(set = 0, binding = 1) uniform sampler2D NormalMap;

void main()
{
	OutFragColor =  texture(Diffuse, InTexCoord) * InColor;
}