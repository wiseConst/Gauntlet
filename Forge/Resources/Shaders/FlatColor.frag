#version 460

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 in_Color;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in flat float in_TextureId;
layout(location = 3) in vec3 in_Normal;

layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_FragColor;

layout(set = 0, binding = 0) uniform sampler2D u_Sprites[32];

void main()
{
	int textureID = int(in_TextureId);
	if(textureID >= 32) textureID = 0;

	// TODO: 2d normal mapping?
	out_Normal = vec4(in_Normal, 0.0);

	const vec4 spriteTex = texture(u_Sprites[nonuniformEXT(textureID)], in_TexCoord);
	if(spriteTex.a < 0.00001) discard;
	out_FragColor = spriteTex * in_Color;
}