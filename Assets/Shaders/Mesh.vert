#version 460

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec4 InColor;
layout(location = 3) in vec2 InTexCoord;
layout(location = 4) in vec3 InTangent;
layout(location = 5) in vec3 InBitangent;

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec3 OutNormal;
layout(location = 2) out vec2 OutTexCoord;
layout(location = 3) out vec3 OutTangent;
layout(location = 4) out vec3 OutBitangent;

layout( push_constant ) uniform PushConstants
{	
	mat4 RenderMatrix;
	vec4 Color;
} MeshPushConstants;

void main()
{
	gl_Position = MeshPushConstants.RenderMatrix * vec4(InPosition, 1.0f);

	OutColor = InColor;
	OutNormal = InNormal;
	OutTexCoord = InTexCoord;
	OutTangent = InTangent;
	OutBitangent = InBitangent;
}