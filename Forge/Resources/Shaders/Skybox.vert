#version 460 

layout(location = 0) in vec3 InPosition;

layout(location = 0) out vec3 OutUVW;

layout( push_constant ) uniform PushConstants
{
	mat4 TransformMatrix; // there's mvp * transform (we use this to prevent creating UBO)
	vec4 Data;
} u_SkyboxPushConstants;

void main()
{
	gl_Position = u_SkyboxPushConstants.TransformMatrix * vec4(InPosition, 1.0);
	OutUVW = InPosition;
	OutUVW.xy*=-1.0;
}
