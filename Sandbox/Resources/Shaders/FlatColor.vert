#version 460

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec3 InColor;

layout(location = 0) out vec3 OutColor;

layout( push_constant ) uniform PushConstants
{	
	mat4 RenderMatrix;
	vec4 Data;
} MeshPushConstants;

void main()
{
	gl_Position = MeshPushConstants.RenderMatrix * vec4(InPosition, 1.0f);
	OutColor = InColor;
}