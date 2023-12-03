#version 460

layout(location = 0) in vec3 in_Pos;
layout(location = 1) in vec3 in_Velocity;
layout(location = 2) in vec3 in_Color;

layout(location = 0) out vec3 out_Color;
layout(location = 1) out vec3 out_FragPos;

layout(push_constant) uniform PushConstants
{
	mat4 CameraProjection;
	mat4 CameraView;
} u_ParticlePushConstants;

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main()
{
	out_FragPos = in_Pos;
	gl_Position = u_ParticlePushConstants.CameraProjection * u_ParticlePushConstants.CameraView * vec4(out_FragPos, 1.0f); 
	gl_PointSize = 8.0f;

	out_Color = in_Color;
}