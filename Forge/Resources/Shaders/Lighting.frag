#version 460

layout(location = 0) in vec2 InTexCoord;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D PositionMap;
layout(set = 0, binding = 1) uniform sampler2D NormalMap;
layout(set = 0, binding = 2) uniform sampler2D AlbedoMap;
layout(set = 0, binding = 3) uniform sampler2D ShadowMap;
// layout(set = 0, binding = 4) uniform sampler2D SSAOMap;

layout( push_constant ) uniform PushConstants
{	
	mat4 TransformMatrix; // And here I store LightSpaceMatrix
	vec4 Color; // Here I store cameraViewPosition
} MeshPushConstants;

struct DirectionalLight
{
    vec4 Color;
    vec4 Direction;
    vec4 AmbientSpecularShininess;
	// bool bCastShadows;
};

struct PointLight
{
	vec4 Position;
	vec4 Color;
	vec4 AmbientSpecularShininess;
	vec4 CLQ; // Constant Linear Quadratic [ Attenuation part https://learnopengl.com/Lighting/Light-casters ]
};

#define MAX_POINT_LIGHTS 16

layout(set = 0, binding = 4) uniform LightingModelBuffer
{
	DirectionalLight DirLight;
	PointLight PointLights[MAX_POINT_LIGHTS];

	float Gamma;
} InLightingModelBuffer;

float CalculateShadows(const vec4 LightSpaceFragPos, const vec3 UnitNormal, const vec3 LightDirection)
{
	// Transforming into NDC [-1, 1]
	vec3 projCoords = LightSpaceFragPos.xyz / LightSpaceFragPos.w; 

	// DepthMap's range is [0, 1] => transform from NDC to screen space [0, 1]
	projCoords = projCoords * 0.5 + 0.5;

	const float currentDepth = projCoords.z;
	const float closestDepth = texture(ShadowMap, projCoords.xy).r;

	const float bias = max(0.05f * (1.0f - dot(UnitNormal, LightDirection)), 0.005f);
	const float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f; 
	return shadow;
}

vec3 CalculateDirectionalLight(const float ssao, const DirectionalLight DirLight, const vec3 ViewVector, const vec3 UnitNormal, const float shadow)
{
	// Ambient
	const vec3 AmbientColor = ssao * DirLight.AmbientSpecularShininess.x * vec3(DirLight.Color);

	const vec3 NormalizedLightDirection = normalize(vec3(-DirLight.Direction));

	// Diffuse
	const float DiffuseFactor = max(dot(UnitNormal, NormalizedLightDirection), 0.0);
	const vec3 DiffuseColor = DiffuseFactor * vec3(DirLight.Color);	

	// Specular Blinn-Phong
	const vec3 HalfwayDir = normalize(-ViewVector + NormalizedLightDirection);
	const float SpecularFactor = pow(max(dot(HalfwayDir, UnitNormal), 0.0), DirLight.AmbientSpecularShininess.z);
	const vec3 SpecularColor = DirLight.AmbientSpecularShininess.y * SpecularFactor * vec3(DirLight.Color);  

	return AmbientColor + (1.0f - shadow) * (DiffuseColor + SpecularColor);
}

vec3 CalculatePointLightColor(PointLight InPointLight, const vec3 FragPos, const vec3 ViewVector, const vec3 UnitNormal) {
	// Ambient
	const vec3 AmbientColor = vec3(InPointLight.AmbientSpecularShininess.x * vec3(InPointLight.Color));
	
	// Point light direction towards light source
	const vec3 NormalizedLightDirection = normalize(vec3(InPointLight.Position) - FragPos);
	
	// Diffuse
	const float DiffuseFactor = max(dot(UnitNormal, NormalizedLightDirection), 0.0);
	const vec3 DiffuseColor = DiffuseFactor * vec3(InPointLight.Color);
	
	// Specular Blinn-Phong
	const vec3 HalfwayDir = normalize(-ViewVector + NormalizedLightDirection);
	const float SpecularFactor = pow(max(dot(HalfwayDir, UnitNormal), 0.0), InPointLight.AmbientSpecularShininess.z);
	const vec3 SpecularColor = InPointLight.AmbientSpecularShininess.y * SpecularFactor * vec3(InPointLight.Color);  
	
	// Attenuation based on distance
	const float Distance = length(vec3(InPointLight.Position) - FragPos);
	const float Attenutaion = 1.0 / (InPointLight.CLQ.x + InPointLight.CLQ.y * Distance + InPointLight.CLQ.z * (Distance * Distance));

	return Attenutaion * (AmbientColor + DiffuseColor + SpecularColor);
}

void main()
{
	const vec3 FragPos = texture(PositionMap, InTexCoord).rgb;
    const vec3 Normal =  texture(NormalMap,   InTexCoord).rgb;
    const vec4 Albedo =  texture(AlbedoMap,   InTexCoord);
	const float ssao =   1.0f;//texture(SSAOMap,     InTexCoord).r;

	// TEMPORARY
	if(Albedo.x == 0.0)
	{
		OutFragColor = vec4(FragPos, 1.0);
	} else
	{

	vec4 FinalColor = vec4(Albedo.rgb, 1.0);

	const vec3 ViewVector = normalize(FragPos - MeshPushConstants.Color.xyz);

	const vec4 LightSpaceFragPos = MeshPushConstants.TransformMatrix * vec4(FragPos, 1.0);
	
	float dirShadow = 0.0f;
//	if(InLightingModelBuffer.DirLight.bCastShadows)
//	{
		dirShadow = CalculateShadows(LightSpaceFragPos, Normal, vec3(InLightingModelBuffer.DirLight.Direction));
	//}
	FinalColor.rgb += CalculateDirectionalLight(ssao, InLightingModelBuffer.DirLight, ViewVector, Normal, dirShadow);
	
	for(int i = 0; i < MAX_POINT_LIGHTS; ++i)
		FinalColor.rgb += CalculatePointLightColor(InLightingModelBuffer.PointLights[i], FragPos, ViewVector, Normal);

	// FinalColor.rgb = FinalColor.rgb / (FinalColor.rgb + vec3(1.0));
	FinalColor.rgb = pow(FinalColor.rgb, vec3(1.0 / InLightingModelBuffer.Gamma));
	OutFragColor = FinalColor;
	}
}