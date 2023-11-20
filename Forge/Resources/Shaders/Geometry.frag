#version 460

layout(location = 0) out vec4 out_Position;
layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_Albedo;
layout(location = 3) out vec4 out_MRAO; // Metallic Roughness AO

layout(location = 0) in vec4 in_Color;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_FragmentPosition;
layout(location = 3) in mat3 in_TBN;

layout(set = 0, binding = 0) uniform sampler2D u_Albedo;
layout(set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout(set = 0, binding = 2) uniform sampler2D u_MetallicMap;
layout(set = 0, binding = 3) uniform sampler2D u_RoughnessMap;
layout(set = 0, binding = 4) uniform sampler2D u_AOMap;

layout(set = 0, binding = 6) uniform UBMaterialData
{
	vec4 BaseColor;
    float Metallic;
    float Roughness;
    float padding0;
    float padding1;
} u_PBRMaterial;

void main()
{
    out_Albedo = texture(u_Albedo, in_TexCoord) * in_Color * u_PBRMaterial.BaseColor;
	if (out_Albedo.a < 0.00001) discard; // Temporary "alpha-blending"
	
    out_Position = vec4(in_FragmentPosition, 1.0);
	
	// Transforming normal map from tangent space to world space.
	const vec3 N = in_TBN * normalize(texture(u_NormalMap, in_TexCoord).rgb * 2.0 - 1.0);
    out_Normal = normalize(vec4(N, 1.0));

	const float Metallic = texture(u_MetallicMap,   in_TexCoord).r * u_PBRMaterial.Metallic;
	const float Roughness = texture(u_RoughnessMap, in_TexCoord).r * u_PBRMaterial.Roughness;
	const float AO = texture(u_AOMap, in_TexCoord).r;
	out_MRAO = vec4(Metallic, Roughness, AO, 0.0);
}