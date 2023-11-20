#version 460

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec4 in_Color;
layout(location = 2) in vec2 in_TexCoord;
layout(location = 3) in vec3 in_Normal;
layout(location = 4) in vec3 in_Tangent;

layout(location = 0) out vec4 out_Color;
layout(location = 1) out vec2 out_TexCoord;
layout(location = 2) out vec3 out_FragmentPosition;
layout(location = 3) out mat3 out_TBN;

layout( push_constant ) uniform PushConstants
{	
	mat4 TransformMatrix; // model matrix here
	mat4 NormalMatrix;    // transpose(inverse(modelMatrix)) calculation on CPU
} u_MeshPushConstants;

layout(set = 0, binding = 5) uniform CameraDataBuffer
{
	mat4 Projection;
	mat4 View;
	vec3 Position;
} u_CameraDataBuffer;

void main()
{
	out_FragmentPosition = (u_MeshPushConstants.TransformMatrix * vec4(in_Position, 1.0)).xyz;
	gl_Position = u_CameraDataBuffer.Projection * u_CameraDataBuffer.View * vec4(out_FragmentPosition, 1.0);

	out_Color = in_Color;
	out_TexCoord = in_TexCoord;

	// In case we scaled/rotated our model -> transform to world space.
	//const mat3 mNormal = transpose(inverse(mat3(u_MeshPushConstants.TransformMatrix)));
	const mat3 mNormal = mat3(u_MeshPushConstants.NormalMatrix);

	// Calculate TBN, to transform NormalMap from tangent space into world(model) space.
	const vec3 N = normalize(mNormal * normalize(in_Normal));
	const vec3 T = normalize(mNormal * normalize(in_Tangent));
	const vec3 B = cross(N, T);
	out_TBN = mat3(T, B, N);
	// NormalMap always oriented along X, Y, Z+, 
	// that's why we manage them properly like this in case object was transformed.
}