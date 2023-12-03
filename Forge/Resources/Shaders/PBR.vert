#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec4 InColor;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InNormal;
layout(location = 4) in vec3 InTangent;

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec3 OutNormal;
layout(location = 2) out vec2 OutTexCoord;
layout(location = 3) out vec3 OutFragmentPosition;
layout(location = 4) out vec3 OutViewVector;

layout( push_constant ) uniform PushConstants
{	
	mat4 TransformMatrix; // model matrix here
	vec4 Data;
} u_MeshPushConstants;

layout(set = 0, binding = 5) uniform CameraDataBuffer
{
	mat4 Projection;
	mat4 View;
	vec3 Position;
} u_CameraDataBuffer;

void main()
{
	OutFragmentPosition = (u_MeshPushConstants.TransformMatrix * vec4(InPosition, 1.0)).xyz;
	gl_Position = u_CameraDataBuffer.Projection * u_CameraDataBuffer.View * vec4(OutFragmentPosition, 1.0);

	OutColor = InColor;
	OutTexCoord = InTexCoord;

	const mat3 mNormal = transpose(inverse(mat3(u_MeshPushConstants.TransformMatrix)));
	OutNormal = mNormal * normalize(InNormal);
}