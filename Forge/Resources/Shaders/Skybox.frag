#version 460 

layout(location = 0) in vec3 InUVW;

layout(set = 0, binding = 0) uniform samplerCube u_CubeMap;

layout(location = 0) out vec4 OutFragColor;

void main()
{ 
	OutFragColor = texture(u_CubeMap, InUVW);
}