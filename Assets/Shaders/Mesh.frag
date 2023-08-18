#version 460

layout(location = 0) in vec4 InColor;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InTangent;
layout(location = 4) in vec3 InBitangent;
layout(location = 5) in vec3 InViewVector;
layout(location = 6) in vec3 InFragmentPosition;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D Diffuse;
layout(set = 0, binding = 1) uniform sampler2D NormalMap;
layout(set = 0, binding = 2) uniform sampler2D Emissive;
layout(set = 0, binding = 3) uniform samplerCube EnvironmentMap;

layout(set = 0, binding = 5) uniform PhongModelBuffer
{
	vec4 LightPosition;
	vec4 LightColor;
	vec4 AmbientSpecularShininessGamma;

	// Attenuation part https://learnopengl.com/Lighting/Light-casters
	float Constant;
	float Linear;
	float Quadratic;
} InPhongModelBuffer;

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

	const float Distance = length(vec3(InPhongModelBuffer.LightPosition) - InFragmentPosition);
	const float Attenutaion = 1.0f / (InPhongModelBuffer.Constant + InPhongModelBuffer.Linear * Distance + InPhongModelBuffer.Quadratic *(Distance * Distance));

	const vec3 InUnitNormal = normalize(InNormal);
	const vec3 NormalizedLightDirection = normalize(vec3(InPhongModelBuffer.LightPosition) - InFragmentPosition);
	
	const float DiffuseFactor = max(dot(InUnitNormal, NormalizedLightDirection), 0.0);
	const vec3 DiffuseColor = DiffuseFactor * vec3(InPhongModelBuffer.LightColor);

	const vec3 ReflectDir = reflect(-NormalizedLightDirection, InUnitNormal);

	const float SpecularFactor = pow(max(dot(-InViewVector, ReflectDir), 0.0), InPhongModelBuffer.AmbientSpecularShininessGamma.z);
	const vec3 SpecularColor = InPhongModelBuffer.AmbientSpecularShininessGamma.y * SpecularFactor * vec3(InPhongModelBuffer.LightColor);  

	const vec3 AmbientColor = vec3(InPhongModelBuffer.AmbientSpecularShininessGamma.x * vec3(InPhongModelBuffer.LightColor));
	vec4 FinalColor = Attenutaion * FinalTexture * vec4(AmbientColor + DiffuseColor + SpecularColor, 1.0f); // mix(EnvironmentMapTexture, FinalTexture , 0.95f);
	
	FinalColor.rgb = pow(FinalColor.rgb, vec3(1.0f/InPhongModelBuffer.AmbientSpecularShininessGamma.w));
	OutFragColor = FinalColor;
}