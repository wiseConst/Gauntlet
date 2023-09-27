#version 460

layout(location = 0) out vec4 OutPosition;
layout(location = 1) out vec4 OutNormal;
layout(location = 2) out vec4 OutAlbedo;

layout(location = 0) in vec4 InColor;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InFragmentPosition;
layout(location = 4) in vec3 InTangent;
layout(location = 5) in vec3 InBitangent;

layout(set = 0, binding = 0) uniform sampler2D Albedo;
layout(set = 0, binding = 1) uniform sampler2D NormalMap;

void main()
{
    OutPosition = vec4(InFragmentPosition, 1.0);
 
 	// Calculate normal in tangent space
	vec3 T = normalize(InTangent);
	vec3 B = normalize(InBitangent);
	vec3 N = normalize(InNormal);
	mat3 TBN = mat3(T, B, N);
	vec3 normal = TBN * normalize(texture(NormalMap, InTexCoord).xyz * 2.0 - vec3(1.0));
    OutNormal = normalize(vec4(normal, 1.0));

    OutAlbedo = texture(Albedo, InTexCoord) * InColor;
}