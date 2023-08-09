#version 460

layout(location = 0) in vec4 InColor;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InTangent;
layout(location = 4) in vec3 InBitangent;
layout(location = 5) in vec3 InViewVector;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D Diffuse;
layout(set = 0, binding = 1) uniform sampler2D NormalMap;
layout(set = 0, binding = 2) uniform sampler2D Emissive;
layout(set = 0, binding = 3) uniform samplerCube EnvironmentMap;

void main()
{
	vec3 BumpNormal = texture(NormalMap, InTexCoord).rgb;
	BumpNormal = normalize(BumpNormal * 2.0 - 1.0);

	const vec3 UnitNormal = normalize(BumpNormal * InNormal);
	const vec3 ReflectedVector = reflect(InViewVector, UnitNormal);

	const vec4 EnvironmentMapTexture = texture(EnvironmentMap, ReflectedVector);
	const vec4 DiffuseTexture = texture(Diffuse, InTexCoord);
	const vec4 EmissiveTexture = texture(Emissive, InTexCoord);

	vec4 FinalTexture = vec4(vec3(0.0f), 1.0f);
	if(DiffuseTexture.r != 1.0f && DiffuseTexture.g != 1.0f && DiffuseTexture.b != 1.0f && EmissiveTexture.r != 1.0f && EmissiveTexture.g != 1.0f && EmissiveTexture.b != 1.0f)
	{
		FinalTexture = vec4(EmissiveTexture.rgb + DiffuseTexture.rgb , 1.0f);
	}
	else if(DiffuseTexture.r == 1.0f && DiffuseTexture.g == 1.0f && DiffuseTexture.b == 1.0f)
	{
		FinalTexture = vec4(EmissiveTexture.rgb, 1.0f);
	}
	else
	{
		FinalTexture = DiffuseTexture;
	}

	const vec4 FinalColor = mix(EnvironmentMapTexture, FinalTexture, 0.95f);
	OutFragColor = FinalColor;
}