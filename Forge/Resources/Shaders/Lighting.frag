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

struct SpotLight
{
   vec4 Position;
   vec4 Color;
   vec3 Direction;
   float CutOff;

   float Ambient;
   float Specular;
   float Shininess;
   int Active;
};

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8
#define MAX_DIR_LIGHTS 4

layout(set = 0, binding = 5) uniform UBBlinnPhong
{
	DirectionalLight DirLights[MAX_DIR_LIGHTS];
	SpotLight SpotLights[MAX_SPOT_LIGHTS];
	PointLight PointLights[MAX_POINT_LIGHTS];

	float Gamma;
	float Exposure;
} u_LightingModelBuffer;

float CalculateShadows(const vec4 LightSpaceFragPos, const vec3 UnitNormal, const vec3 LightDirection)
{
	// Transforming into NDC [-1, 1]
	vec3 projCoords = LightSpaceFragPos.xyz / LightSpaceFragPos.w; 

	// DepthMap's range is [0, 1] => transform from NDC to screen space [0, 1]
	projCoords = projCoords * 0.5 + 0.5;
	if(projCoords.z > 1.0) return  0.0f;

	const float currentDepth = projCoords.z;
	const float closestDepth = texture(u_ShadowMap, projCoords.xy).r;

	const vec3 normalizedLightDirection = normalize(-LightDirection);
	const float bias = max(0.05 * (1.0 - dot(UnitNormal, normalizedLightDirection)), 0.005);

	const vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
	float shadow = 0.0f;
	for(int x = -1; x <= 1; ++x)
	{
		for(int y= -1; y<=1;++y)
		{
			const float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;    
		}
	}
	shadow /= 9.0;

	return shadow;
}

vec3 CalculateDirectionalLight(const DirectionalLight DirLight, const vec3 ViewVector, const vec3 UnitNormal, const float shadow)
{
	vec3 AmbientColor = DirLight.AmbientSpecularShininessCastShadows.x * vec3(DirLight.Color);
	
	const vec3 NormalizedLightDirection = normalize(-DirLight.Direction.xyz);

	const float DiffuseFactor = max(dot(UnitNormal, NormalizedLightDirection), 0.0);
	const vec3 DiffuseColor = DiffuseFactor * vec3(DirLight.Color);	

	const vec3 HalfwayDir = normalize(-ViewVector + NormalizedLightDirection);
	const float SpecularFactor = pow(max(dot(HalfwayDir, UnitNormal), 0.0), DirLight.AmbientSpecularShininessCastShadows.z);
	const vec3 SpecularColor = DirLight.AmbientSpecularShininessCastShadows.y * SpecularFactor * vec3(DirLight.Color);  

	return AmbientColor + (1.0f - shadow) * (DiffuseColor + SpecularColor);
}

vec3 CalculateSpotLight(const SpotLight spotlight, const vec3 fragPos, const vec3 unitNormal, const vec3 viewVector)
{
	const vec3 ambient = spotlight.Ambient * spotlight.Color.xyz;
	const vec3 lightDir = normalize(spotlight.Position.xyz - fragPos);
	
	const float diff = max(dot(unitNormal, lightDir), 0.0);
	const vec3 diffuse = diff * spotlight.Color.xyz;
	
	const vec3 HalfwayDir = normalize(-viewVector + lightDir);
	const float spec = pow(max(dot(HalfwayDir, unitNormal), 0.0), spotlight.Shininess);
	const vec3 specular = spotlight.Specular * spec * spotlight.Color.xyz;  

	const float angle = dot(spotlight.Direction, -lightDir);
	const float intensity = clamp(angle - spotlight.CutOff, 0.0, 1.0);

	return ambient + intensity * (diffuse + specular);

//	const vec3 lightDir = normalize(spotlight.Position.xyz - fragPos);
//	const float theta = dot(lightDir, normalize(-spotlight.Direction));
//	if(theta > spotlight.CutOff) 
//	{       
//	    const vec3 ambient = spotlight.Ambient * spotlight.Color.xyz;
//    
//		const float diff = max(dot(unitNormal, lightDir), 0.0);
//		const vec3 diffuse = diff * spotlight.Color.xyz;
//
//		const vec3 halfwayDir = normalize(-viewVector + lightDir);
//		const float spec = pow(max(dot(halfwayDir, unitNormal), 0.0), spotlight.Shininess);
//		const vec3 specular = spotlight.Specular * spec * spotlight.Color.xyz;
//
//        // TODO: Attenuation
//        return /* attenuation * */ (ambient + diffuse + specular);
//	}
//	
//	return spotlight.Ambient * spotlight.Color.xyz;
}

vec3 CalculatePointLight(PointLight InPointLight, const vec3 FragPos, const vec3 ViewVector, const vec3 UnitNormal) {
	const vec3 AmbientColor = vec3(InPointLight.AmbientSpecularShininess.x * vec3(InPointLight.Color));
	const vec3 NormalizedLightDirection = normalize(vec3(InPointLight.Position) - FragPos);
	
	const float DiffuseFactor = max(dot(UnitNormal, NormalizedLightDirection), 0.0);
	const vec3 DiffuseColor = DiffuseFactor * vec3(InPointLight.Color);
	
	const vec3 HalfwayDir = normalize(-ViewVector + NormalizedLightDirection);
	const float SpecularFactor = pow(max(dot(HalfwayDir, UnitNormal), 0.0), InPointLight.AmbientSpecularShininess.z);
	const vec3 SpecularColor = InPointLight.AmbientSpecularShininess.y * SpecularFactor * vec3(InPointLight.Color);  
	
	const float dist = length(vec3(InPointLight.Position) - FragPos);
	const float Attenutaion = 1.0 / (InPointLight.CLQActive.x + InPointLight.CLQActive.y * dist + InPointLight.CLQActive.z * (dist * dist));

	return Attenutaion * (AmbientColor + DiffuseColor + SpecularColor);
}

void main()
{
    const vec4 Albedo =  texture(u_AlbedoMap,   InTexCoord);
    if(Albedo.a == 0.0) discard;

    const vec3 Normal =  texture(u_NormalMap,   InTexCoord).rgb;
	if(Normal == vec3(0.0)) { 
		OutFragColor = Albedo;
		return;
	}

    const vec3 FragPos = texture(u_PositionMap, InTexCoord).rgb;
    const float ssao =   texture(u_SSAOMap,     InTexCoord).r;

	vec4 FinalColor = vec4(Albedo.rgb  * ssao  * 0.45, 1.0);
	const vec3 ViewVector = normalize(FragPos - u_MeshPushConstants.Data.xyz);
	
	const vec4 LightSpaceFragPos = u_MeshPushConstants.TransformMatrix * vec4(FragPos, 1.0);
	for(int i = 0; i < MAX_DIR_LIGHTS; ++i)
	{
		float dirShadow = 0.0f;
		
		if(u_LightingModelBuffer.DirLights[i].AmbientSpecularShininessCastShadows.w == 1.0f)
		{
			dirShadow = CalculateShadows(LightSpaceFragPos, Normal, u_LightingModelBuffer.DirLights[i].Direction.xyz);
		}
	
		if(u_LightingModelBuffer.DirLights[i].Color.xyz != vec3(0.0)) 
			FinalColor.rgb += CalculateDirectionalLight(u_LightingModelBuffer.DirLights[i], ViewVector, Normal, dirShadow);
	}

	for(int i = 0; i < MAX_SPOT_LIGHTS; ++i)
	{
		if(u_LightingModelBuffer.SpotLights[i].Active == 1) FinalColor.rgb += CalculateSpotLight(u_LightingModelBuffer.SpotLights[i], FragPos, Normal, ViewVector);
	}

	for(int i = 0; i < MAX_POINT_LIGHTS; ++i)
	{
		if(u_LightingModelBuffer.PointLights[i].CLQActive.w == 0.0) continue;
		
		FinalColor.rgb += CalculatePointLight(u_LightingModelBuffer.PointLights[i], FragPos, ViewVector, Normal);
	}

	// reinhard tone mapping
	FinalColor.rgb = FinalColor.rgb / (FinalColor.rgb + vec3(1.0));

	FinalColor.rgb = pow(FinalColor.rgb, vec3(1.0 / u_LightingModelBuffer.Gamma));
	OutFragColor = vec4(FinalColor.rgb, 1.0);
}