#version 460

layout(location = 0) in vec4 InColor;

layout(location = 0) out vec4 OutFragColor;

void main()
{
	OutFragColor = InColor;
}