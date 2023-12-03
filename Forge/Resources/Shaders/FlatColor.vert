#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3  in_Position;
layout(location = 1) in vec4  in_Color;
layout(location = 2) in vec2  in_TexCoord;
layout(location = 3) in float in_TextureId;
layout(location = 4) in vec3  in_Normal;

layout(location = 0) out vec4 out_Color;
layout(location = 1) out vec2 out_TexCoord;
layout(location = 2) out flat float out_TextureId;
layout(location = 3) out vec3 out_Normal;

layout( push_constant ) uniform PushConstants
{	
	mat4 RenderMatrix;
	vec4 Data;
} u_MeshPushConstants;

void main()
{
	gl_Position = u_MeshPushConstants.RenderMatrix * vec4(in_Position, 1.0f);

	out_Color     = in_Color;
	out_TexCoord  = in_TexCoord;
	out_TextureId = in_TextureId;
	out_Normal    = in_Normal;
}