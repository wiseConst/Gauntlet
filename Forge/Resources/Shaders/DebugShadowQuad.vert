#version 460

// From Sascha Willems

layout (location = 0) out vec2 outUV;

void main() 
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2); // Insane coordinates generation.
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
	outUV.xy *= -1.0f;
}
