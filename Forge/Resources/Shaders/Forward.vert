#version 460

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec4 InColor;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InNormal;
layout(location = 4) in vec3 InTangent;

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec3 OutNormal;
layout(location = 2) out vec2 OutTexCoord;
layout(location = 3) out vec3 OutTangent;
layout(location = 4) out vec3 OutBitangent;
layout(location = 5) out vec3 OutViewVector;
layout(location = 6) out vec3 OutFragmentPosition;
layout(location = 7) out vec4 OutLightSpaceFragPos;

layout( push_constant ) uniform PushConstants
{	
	mat4 TransformMatrix;
	vec4 Data;
} u_MeshPushConstants;

layout(set = 0, binding = 4) uniform CameraDataBuffer
{
	mat4 Projection;
	mat4 View;
	vec3 Position;
} u_CameraDataBuffer;

layout(set = 0, binding = 7) uniform ShadowsBuffer
{
	mat4 LightSpaceMatrix;
} u_ShadowsBuffer;

void main()
{
	const vec4 VertexWorldPosition = u_MeshPushConstants.TransformMatrix * vec4(InPosition, 1.0f);
	const mat4 CameraViewProjectionMatrix = u_CameraDataBuffer.Projection * u_CameraDataBuffer.View;
	gl_Position = CameraViewProjectionMatrix * VertexWorldPosition;

	OutColor = InColor;
	// We also have to take in consideration that normals can change in case we did some rotation, but they aren't affected by model translation that's why we take mat3
	OutNormal = mat3(transpose(inverse(u_MeshPushConstants.TransformMatrix))) * InNormal;
	OutTexCoord = InTexCoord;
	OutTangent = InTangent;
	OutBitangent = cross(InNormal, OutTangent);
	OutViewVector = normalize(VertexWorldPosition.xyz - u_CameraDataBuffer.Position);
	OutFragmentPosition = VertexWorldPosition.xyz;
	OutLightSpaceFragPos = u_ShadowsBuffer.LightSpaceMatrix * vec4(OutFragmentPosition, 1.0);
}
