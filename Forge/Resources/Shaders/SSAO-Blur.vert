#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec2 out_TexCoord;

void main()
{
	out_TexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(out_TexCoord * 2.0f - 1.0f, 0.0f, 1.0f);
	out_TexCoord.y *= -1.0f;
}