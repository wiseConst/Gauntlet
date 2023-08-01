#version 460

layout(location = 0) in vec4 InColor;
layout(location = 1) in vec2 InTexCoord;
layout(location = 2) in float InTextureId;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D Sprites[32];

void main()
{
	int TextureId = int(InTextureId);
	if(TextureId >= 32) TextureId = 0;

	OutFragColor = texture(Sprites[TextureId], InTexCoord) * InColor;
}