#version 460

layout(location = 0) in vec2 InTexCoord;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D u_PositionMap;
layout(set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout(set = 0, binding = 2) uniform sampler2D u_AlbedoMap;
layout(set = 0, binding = 3) uniform sampler2D u_ShadowMap;
layout(set = 0, binding = 4) uniform sampler2D u_SSAOMap;

layout( push_constant ) uniform PushConstants
{	
	mat4 TransformMatrix; // And here I store LightSpaceMatrix
	vec4 Data; // Here I store cameraViewPosition
} u_MeshPushConstants;

struct DirectionalLight
{
    vec4 Color;
    vec4 Direction;
    vec4 AmbientSpecularShininessCastShadows;
};

struct PointLight
{
	vec4 Position;
	vec4 Color;
	vec4 AmbientSpecularShininess;
	vec4 CLQActive; // Constant Linear Quadratic IsActive
};

#define MAX_POINT_LIGHTS 16

layout(set = 0, binding = 5) uniform LightingModelBuffer
{
	DirectionalLight DirLight;
	PointLight PointLights[MAX_POINT_LIGHTS];

	float Gamma;
} u_LightingModelBuffer;

float CalculateShadows(const vec4 LightSpaceFragPos, const vec3 UnitNormal, const vec3 LightDirection)
{
	// Transforming into NDC [-1, 1]
	vec3 projCoords = LightSpaceFragPos.xyz / LightSpaceFragPos.w; 

	// DepthMap's range is [0, 1] => transform from NDC to screen space [0, 1]
	projCoords = projCoords * 0.5 + 0.5;

	const float currentDepth = projCoords.z;
	const float closestDepth = texture(u_ShadowMap, projCoords.xy).r;

	const vec3 normalizedLightDirection = normalize(-LightDirection);
	const float bias = max(0.05f * (1.0f - dot(UnitNormal, normalizedLightDirection)), 0.005f);
	float shadow = 0.0f;
	const vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);

	for(int x = -1; x <= 1; ++x)
	{
		for(int y= -1; y<=1;++y)
		{
			const float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;    
		}
	}
	shadow /= 9.0;

	if(projCoords.z > 1.0) shadow = 0.0f;

	return shadow;
}

vec3 CalculateDirectionalLight(const float ssao, const DirectionalLight DirLight, const vec3 ViewVector, const vec3 UnitNormal, const float shadow)
{
	// Ambient
	const vec3 AmbientColor = ssao * DirLight.AmbientSpecularShininessCastShadows.x * vec3(DirLight.Color);

	const vec3 NormalizedLightDirection = normalize(-DirLight.Direction.xyz);

	// Diffuse
	const float DiffuseFactor = max(dot(UnitNormal, NormalizedLightDirection), 0.0);
	const vec3 DiffuseColor = DiffuseFactor * vec3(DirLight.Color);	

	// Specular Blinn-Phong
	const vec3 HalfwayDir = normalize(-ViewVector + NormalizedLightDirection);
	const float SpecularFactor = pow(max(dot(HalfwayDir, UnitNormal), 0.0), DirLight.AmbientSpecularShininessCastShadows.z);
	const vec3 SpecularColor = DirLight.AmbientSpecularShininessCastShadows.y * SpecularFactor * vec3(DirLight.Color);  

	return AmbientColor + (1.0f - shadow) * (DiffuseColor + SpecularColor);
}

vec3 CalculatePointLightColor(const float ssao, PointLight InPointLight, const vec3 FragPos, const vec3 ViewVector, const vec3 UnitNormal) {
	// Ambient
	const vec3 AmbientColor = ssao * vec3(InPointLight.AmbientSpecularShininess.x * vec3(InPointLight.Color));
	
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
	const float Attenutaion = 1.0 / (InPointLight.CLQActive.x + InPointLight.CLQActive.y * Distance + InPointLight.CLQActive.z * (Distance * Distance));

	return Attenutaion * (AmbientColor + DiffuseColor + SpecularColor);
}

void main()
{
	const vec3 FragPos = texture(u_PositionMap, InTexCoord).rgb;
    const vec3 Normal =  texture(u_NormalMap,   InTexCoord).rgb;
    const vec4 Albedo =  texture(u_AlbedoMap,   InTexCoord);
	const float ssao =   texture(u_SSAOMap,     InTexCoord).r;

	// 2D or SKYBOX case
//	if(FragPos.x == 0.0) 
//	{
//		OutFragColor = vec4(Albedo.rgb, 1.0);
//		return;
//	}


	vec4 FinalColor = vec4(Albedo.rgb  * ssao   * 0.45, 1.0);

	const vec3 ViewVector = normalize(FragPos - u_MeshPushConstants.Data.xyz);

	const vec4 LightSpaceFragPos = u_MeshPushConstants.TransformMatrix * vec4(FragPos, 1.0);
	
	float dirShadow = 0.0f;
	if(u_LightingModelBuffer.DirLight.AmbientSpecularShininessCastShadows.w == 1.0f)
	{
		dirShadow = CalculateShadows(LightSpaceFragPos, Normal, u_LightingModelBuffer.DirLight.Direction.xyz);
	}
	
	if(u_LightingModelBuffer.DirLight.Color.xyz != vec3(0.0)) 
		FinalColor.rgb += CalculateDirectionalLight(ssao, u_LightingModelBuffer.DirLight, ViewVector, Normal, dirShadow);
		
	for(int i = 0; i < MAX_POINT_LIGHTS; ++i)
	{
		if(u_LightingModelBuffer.PointLights[i].CLQActive.w == 0.0f) continue;
		
		FinalColor.rgb += CalculatePointLightColor(ssao, u_LightingModelBuffer.PointLights[i], FragPos, ViewVector, Normal);
	}

	// reinhard tone mapping
	FinalColor.rgb = FinalColor.rgb / (FinalColor.rgb + vec3(1.0));

	FinalColor.rgb = pow(FinalColor.rgb, vec3(1.0 / u_LightingModelBuffer.Gamma));
	OutFragColor = FinalColor;
}