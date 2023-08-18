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
layout(location = 5) out vec3 OutViewVector;
layout(location = 6) out vec3 OutFragmentPosition;

layout( push_constant ) uniform PushConstants
{	
	mat4 TransformMatrix;
	vec4 Color;
} MeshPushConstants;

layout(set = 0, binding = 4) uniform CameraDataBuffer
{
	mat4 Projection;
	mat4 View;
	vec3 Position;
} InCameraDataBuffer;

layout(set = 0, binding = 5) uniform PhongModelBuffer
{
	vec4 LightPosition;
	vec4 LightColor;
	vec4 AmbientSpecularShininessGamma;

	// Attenuation part https://learnopengl.com/Lighting/Light-casters
	float Constant;
	float Linear;
	float Quadratic;
} InPhongModelBuffer;

void main()
{
	const vec4 VertexWorldPosition = MeshPushConstants.TransformMatrix * vec4(InPosition, 1.0f);
	const mat4 CameraViewProjectionMatrix = InCameraDataBuffer.Projection * InCameraDataBuffer.View;
	gl_Position = CameraViewProjectionMatrix * VertexWorldPosition;

	OutColor = InColor;
	// We also have to take in consideration that normals can change in case we did some rotation, but they aren't affected by model translation
	OutNormal = vec3(mat3(transpose(inverse(MeshPushConstants.TransformMatrix))) * InNormal);
	OutTexCoord = InTexCoord;
	OutTangent = InTangent;
	OutBitangent = OutBitangent;
	OutViewVector = normalize(VertexWorldPosition.xyz - InCameraDataBuffer.Position);
	OutFragmentPosition = VertexWorldPosition.xyz;

// Converting to camera space
//	const vec3 T =  normalize((CameraViewProjectionMatrix * vec4(InTangent, 0.0f)).rgb);
//	const vec3 B = normalize((CameraViewProjectionMatrix * vec4(InBitangent, 0.0f)).rgb);
//	const vec3 N = normalize((CameraViewProjectionMatrix * vec4(InNormal, 0.0f)).rgb);
//	const mat3 TBN = mat3(T, B, N);
}