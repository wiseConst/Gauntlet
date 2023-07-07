#version 460

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec4 InColor;
layout(location = 3) in vec2 TexCoord;

layout(location = 0) out vec4 OutColor;

layout( push_constant ) uniform PushConstants
{	
	mat4 RenderMatrix;
	vec4 Color;
} MeshPushConstants;

void main()
{
	gl_Position = MeshPushConstants.RenderMatrix * vec4(InPosition, 1.0f);

	OutColor = InColor;
}