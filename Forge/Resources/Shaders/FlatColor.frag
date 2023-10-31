#version 460

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 InColor;
layout(location = 1) in vec2 InTexCoord;
layout(location = 2) in flat float InTextureId;

layout(location = 1) out vec4 OutNormal;
layout(location = 2) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D u_Sprites[32];

void main()
{
	int TextureId = int(InTextureId);
	if(TextureId >= 32) TextureId = 0;

	OutNormal = vec4(0.0);

	const vec4 spriteTex =  texture(u_Sprites[nonuniformEXT(TextureId)], InTexCoord);
	if(spriteTex.a < 0.01) discard;
	OutFragColor = spriteTex * InColor;
}