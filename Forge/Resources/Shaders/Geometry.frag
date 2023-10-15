#version 460

layout(location = 0) out vec4 OutPosition;
layout(location = 1) out vec4 OutNormal;
layout(location = 2) out vec4 OutAlbedo;

layout(location = 0) in vec4 InColor;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InFragmentPosition;
// layout(location = 4) in vec3 InTangent;
// layout(location = 5) in vec3 InBitangent;
layout(location = 6) in mat3 InTBN;

layout(set = 0, binding = 0) uniform sampler2D u_Albedo;
layout(set = 0, binding = 1) uniform sampler2D u_NormalMap;

void main()
{
    OutPosition = vec4(InFragmentPosition, 1.0);
 
	vec3 normal = InTBN * normalize(texture(u_NormalMap, InTexCoord).xyz * 2.0 - vec3(1.0));
    OutNormal = normalize(vec4(normal, 1.0));

    OutAlbedo = texture(u_Albedo, InTexCoord) * InColor;
}