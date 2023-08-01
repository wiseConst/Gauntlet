#version 460 

layout(location = 0) in vec3 InPosition;

layout(location = 0) out vec3 OutUVW;

layout( push_constant ) uniform PushConstants
{
	mat4 RenderMatrix;
	vec4 Color;
} SkyboxPushConstants;

void main()
{
	gl_Position = (SkyboxPushConstants.RenderMatrix * vec4(InPosition, 1.0)).xyww;
	OutUVW = InPosition;
	OutUVW.xy *= -1.0; // because I always flip images whilst loading it using stbi_image
}