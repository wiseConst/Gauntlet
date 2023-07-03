#version 460

layout(location = 0) in vec3 InColor;

layout(location = 0) out vec4 OutFragColor;

void main()
{
	OutFragColor = vec4(InColor, 1.0f);
}