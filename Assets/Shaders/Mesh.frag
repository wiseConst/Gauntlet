#version 460

layout(location = 0) in vec4 InColor;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InTangent;
layout(location = 4) in vec3 InBitangent;
layout(location = 5) in vec3 InViewVector;
layout(location = 6) in vec3 InFragmentPosition;
layout(location = 7) in vec3 InLightPosition;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D Diffuse;
layout(set = 0, binding = 1) uniform sampler2D NormalMap;
layout(set = 0, binding = 2) uniform sampler2D Emissive;
layout(set = 0, binding = 3) uniform samplerCube EnvironmentMap;

const float g_AmbientStrength = 0.1f;
const vec3 g_LightColor = vec3(1.0f);
const vec3 g_AmbientColor = g_LightColor * g_AmbientStrength;

const float g_SpecularStrength = 0.5f;

void main()
{
	vec3 BumpNormal = texture(NormalMap, InTexCoord).rgb;
	BumpNormal = normalize(BumpNormal * 2.0 - 1.0);

	const vec3 UnitNormal = normalize(BumpNormal * InNormal);
	vec3 ReflectedVector = reflect(InViewVector, UnitNormal);
	ReflectedVector.xy *= -1.0f;

	const vec4 EnvironmentMapTexture = texture(EnvironmentMap, ReflectedVector);

	const vec4 DiffuseTexture = texture(Diffuse, InTexCoord);
	const vec4 EmissiveTexture = texture(Emissive, InTexCoord);

	vec4 FinalTexture = vec4(vec3(0.0f), 1.0f);
	if(DiffuseTexture.r != 1.0f && DiffuseTexture.g != 1.0f && DiffuseTexture.b != 1.0f && EmissiveTexture.r != 1.0f && EmissiveTexture.g != 1.0f && EmissiveTexture.b != 1.0f)
	{
		FinalTexture = vec4(EmissiveTexture.rgb + DiffuseTexture.rgb, 1.0f);
	}
	else if(DiffuseTexture.r == 1.0f && DiffuseTexture.g == 1.0f && DiffuseTexture.b == 1.0f)
	{
		FinalTexture = vec4(EmissiveTexture.rgb, 1.0f);
	}
	else
	{
		FinalTexture = DiffuseTexture;
	}

	const vec3 InUnitNormal = normalize(InNormal);
	const vec3 NormalizedLightDirection = normalize(InLightPosition - InFragmentPosition);
	
	const float DiffuseFactor = max(dot(InUnitNormal, NormalizedLightDirection), 0.0);
	const vec3 DiffuseColor = DiffuseFactor * g_LightColor;

	const vec3 ReflectDir = reflect(-NormalizedLightDirection, InUnitNormal);
	const float SpecularFactor = pow(max(dot(-InViewVector, ReflectDir), 0.0), 64);
	const vec3 SpecularColor = g_SpecularStrength * SpecularFactor * g_LightColor;  

	vec4 FinalColor =  FinalTexture * vec4(g_AmbientColor + DiffuseColor + SpecularColor, 1.0f); // mix(EnvironmentMapTexture, FinalTexture , 0.95f);
	
	const float gamma = 2.2f;
	FinalColor.rgb = pow(FinalColor.rgb, vec3(1./gamma));
	OutFragColor = FinalColor;
}