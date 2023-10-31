#version 460

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec4 InColor;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InNormal;
layout(location = 4) in vec3 InTangent;

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec3 OutNormal;
layout(location = 2) out vec2 OutTexCoord;
layout(location = 3) out vec3 OutFragmentPosition;
layout(location = 4) out mat3 OutTBN;

layout( push_constant ) uniform PushConstants
{	
	mat4 TransformMatrix; // model matrix here
	vec4 Data;
} u_MeshPushConstants;

layout(set = 0, binding = 2) uniform CameraDataBuffer
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
	vec3 OutTangent = mNormal * normalize(InTangent);
	vec3 OutBitangent = mNormal * normalize(cross(InNormal, InTangent));

	// Calculate normal in tangent space
	vec3 T = normalize(OutTangent);
	vec3 B = normalize(OutBitangent);
	vec3 N = normalize(OutNormal);
	OutTBN = mat3(T, B, N);
}