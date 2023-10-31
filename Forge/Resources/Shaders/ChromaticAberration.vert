#version 460

layout(location = 0) out vec2 OutTexCoord;

void main()
{
	OutTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(OutTexCoord * 2.0f - 1.0f, 0.0f, 1.0f);

	OutTexCoord.y *= -1.0f; // Because Sascha used CW order, but I use CCW
}