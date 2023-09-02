#version 460

layout(location = 0) in vec4 InColor;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoord;
layout(location = 3) in vec3 InTangent;
layout(location = 4) in vec3 InBitangent;
layout(location = 5) in vec3 InViewVector;
layout(location = 6) in vec3 InFragmentPosition;
layout(location = 7) in vec4 InLightSpaceFragPos;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D Diffuse;
layout(set = 0, binding = 1) uniform sampler2D NormalMap;
layout(set = 0, binding = 2) uniform sampler2D Emissive;
layout(set = 0, binding = 3) uniform samplerCube EnvironmentMap;
layout(set = 0, binding = 6) uniform sampler2D ShadowMap;

struct DirectionalLight
{
    vec4 Color;
    vec4 Direction;
    vec4 AmbientSpecularShininess;
};

struct PointLight
{
	vec4 Position;
	vec4 Color;
	vec4 AmbientSpecularShininess;
	vec4 CLQ; // Constant Linear Quadratic [ Attenuation part https://learnopengl.com/Lighting/Light-casters ]
};

#define MAX_POINT_LIGHTS 16

layout(set = 0, binding = 5) uniform LightingModelBuffer
{
	DirectionalLight DirLight;
	PointLight PointLights[MAX_POINT_LIGHTS];

	float Gamma;
} InLightingModelBuffer;

// simple shadowmapping from here: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
float CalculateShadows(vec4 LightSpaceFragPos)
{
	// Transforming into NDC [-1, 1]
	vec3 projCoords = LightSpaceFragPos.xyz / LightSpaceFragPos.w; 

	// DepthMap's range is [0, 1] => transform from NDC to screen space [0, 1]
	projCoords = projCoords * 0.5 + 0.5;

	const float currentDepth = projCoords.z;
	const float closestDepth = texture(ShadowMap, projCoords.xy).r;

	const float shadow = currentDepth > closestDepth ? 1.0f : 0.0f; 
	return shadow;
}

vec3 CalculateDirectionalLight(const DirectionalLight DirLight, const vec3 UnitNormal, const float shadow)
{
	// Ambient
	const vec3 AmbientColor = DirLight.AmbientSpecularShininess.x * vec3(DirLight.Color);

	// By default dir light direction is coming from light source to fragment
	const vec3 NormalizedLightDirection = normalize(-vec3(DirLight.Direction));

	// Diffuse
	const float DiffuseFactor = max(dot(UnitNormal, NormalizedLightDirection), 0.0);
	const vec3 DiffuseColor = DiffuseFactor * vec3(DirLight.Color);

	// Specular Blinn-Phong
	const vec3 HalfwayDir = normalize(-InViewVector + NormalizedLightDirection);
	const float SpecularFactor = pow(max(dot(HalfwayDir, UnitNormal), 0.0), DirLight.AmbientSpecularShininess.z);
	const vec3 SpecularColor = DirLight.AmbientSpecularShininess.y * SpecularFactor * vec3(DirLight.Color);  

	return AmbientColor + (1.0 - shadow) * (DiffuseColor + SpecularColor);
}

vec3 CalculatePointLightColor(PointLight InPointLight, const vec3 UnitNormal) {
	// Ambient
	const vec3 AmbientColor = vec3(InPointLight.AmbientSpecularShininess.x * vec3(InPointLight.Color));
	
	// Point light direction towards light source
	const vec3 NormalizedLightDirection = normalize(vec3(InPointLight.Position) - InFragmentPosition);
	
	// Diffuse
	const float DiffuseFactor = max(dot(UnitNormal, NormalizedLightDirection), 0.0);
	const vec3 DiffuseColor = DiffuseFactor * vec3(InPointLight.Color);
	
	// Specular Blinn-Phong
	const vec3 HalfwayDir = normalize(-InViewVector + NormalizedLightDirection);
	const float SpecularFactor = pow(max(dot(HalfwayDir, UnitNormal), 0.0), InPointLight.AmbientSpecularShininess.z);
	const vec3 SpecularColor = InPointLight.AmbientSpecularShininess.y * SpecularFactor * vec3(InPointLight.Color);  
	
	// Attenuation based on distance
	const float Distance = length(vec3(InPointLight.Position) - InFragmentPosition);
	const float Attenutaion = 1.0f / (InPointLight.CLQ.x + InPointLight.CLQ.y * Distance + InPointLight.CLQ.z * (Distance * Distance));

	return Attenutaion * (AmbientColor + DiffuseColor + SpecularColor);
}

void main()
{
	vec4 FinalColor = vec4(vec3(0.0f), 1.0f);
	{
		vec3 BumpNormal = texture(NormalMap, InTexCoord).rgb;
		BumpNormal = normalize(BumpNormal * 2.0 - 1.0);

		const vec3 UnitNormal = normalize(BumpNormal * InNormal);
		vec3 ReflectedVector = reflect(InViewVector, UnitNormal);
		ReflectedVector.xy *= -1.0f;

		const vec4 EnvironmentMapTexture = texture(EnvironmentMap, ReflectedVector);

		const vec4 DiffuseTexture = texture(Diffuse, InTexCoord);
		const vec4 EmissiveTexture = texture(Emissive, InTexCoord);

		if(DiffuseTexture.r != 1.0f && DiffuseTexture.g != 1.0f && DiffuseTexture.b != 1.0f && EmissiveTexture.r != 1.0f && EmissiveTexture.g != 1.0f && EmissiveTexture.b != 1.0f)
		{
			FinalColor = vec4(EmissiveTexture.rgb + DiffuseTexture.rgb, 1.0f);
		}
		else if(DiffuseTexture.r == 1.0f && DiffuseTexture.g == 1.0f && DiffuseTexture.b == 1.0f)
		{
			FinalColor = vec4(EmissiveTexture.rgb, 1.0f);
		}
		else
		{
			FinalColor = DiffuseTexture;
		}
	}

	const float shadow = CalculateShadows(InLightSpaceFragPos);

	const vec3 UnitNormal = normalize(InNormal);
	FinalColor.rgb += CalculateDirectionalLight(InLightingModelBuffer.DirLight, UnitNormal, shadow);
	
	for(int i = 0; i < MAX_POINT_LIGHTS; ++i)
		FinalColor.rgb += CalculatePointLightColor(InLightingModelBuffer.PointLights[i], UnitNormal);

	FinalColor.rgb  = FinalColor.rgb / (FinalColor.rgb + vec3(1.0));
	FinalColor.rgb = pow(FinalColor.rgb, vec3(1.0f / InLightingModelBuffer.Gamma)); // OLD: Seems like textures already corrected so no need to use this: 1.0f / InLightingModelBuffer.Gamma, but we should use InLightingModelBuffer.Gamma
	OutFragColor = FinalColor;
}