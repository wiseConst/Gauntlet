#version 460

layout(location = 0) in vec2 in_TexCoord;

layout(location = 0) out vec4 out_FragColor;

layout(set = 0, binding = 0) uniform sampler2D u_PositionMap;
layout(set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout(set = 0, binding = 2) uniform sampler2D u_AlbedoMap;
layout(set = 0, binding = 3) uniform sampler2D u_MRAO; // Metallic Roughness AO
layout(set = 0, binding = 4) uniform sampler2D u_SSAOMap;

layout( push_constant ) uniform PushConstants
{	
	mat4 TransformMatrix;
	vec4 Data; // Here I store cameraViewPosition
} u_MeshPushConstants;

//#include "Common.glsl"

struct DirectionalLight
{
    vec3 Color;
    float Intensity;
    vec3 Direction;
    int CastShadows;
};

struct PointLight
{
    vec4 Position;
    vec4 Color;
    float Ambient;    // useless
    float Specular;   // useless
    float Shininess;  // useless
    int IsActive;
};

struct SpotLight
{
    vec4 Position;
    vec4 Color;
    vec3 Direction;
    float CutOff;

    float Ambient;  // useless
    float Specular;
    float Shininess;  // useless
    int Active;
};

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8
#define MAX_DIR_LIGHTS 4

layout(set = 0, binding = 5) uniform UBLightingData
{
	DirectionalLight DirLights[MAX_DIR_LIGHTS];
	SpotLight SpotLights[MAX_SPOT_LIGHTS];
	PointLight PointLights[MAX_POINT_LIGHTS];

	float Gamma;
	float Exposure;
} u_LightingData;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    const float a = roughness;
    const float a2 = a*a;

    const float NdotH = max(dot(N, H), 0.0);
    const float NdotH2 = NdotH*NdotH;
    const float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    const float r = (roughness + 1.0);
    const float k = (r*r) / 8.0;

    const float nom   = NdotV;
    const float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    const float NdotV = max(dot(N, V), 0.0);
    const float NdotL = max(dot(N, L), 0.0);
    const float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    const float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    const vec3 albedo     = pow(texture(u_AlbedoMap, in_TexCoord).rgb, vec3(2.2));
    const float metallic  = texture(u_MRAO, in_TexCoord).r;
    const float roughness = texture(u_MRAO, in_TexCoord).y;
    const vec3 fragPos = texture(u_PositionMap, in_TexCoord).rgb;
    const float ao        = texture(u_MRAO, in_TexCoord).z;         // this one is from material
    const float calcAO    = texture(u_SSAOMap, in_TexCoord).r; // this one is calculated from scene

    const vec3 N = texture(u_NormalMap, in_TexCoord).rgb;
    const vec3 V = normalize(u_MeshPushConstants.Data.xyz - fragPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    vec3 color = vec3(0.0);

    // constant ambient light
    vec3 ambient = vec3(0.03) * albedo * ao * calcAO;
    for(int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        if(u_LightingData.PointLights[i].IsActive == 0) continue;
        vec3 Lo = vec3(0.0);

        // calculate per-light radiance
        vec3 L = normalize(u_LightingData.PointLights[i].Position.xyz - fragPos);
        vec3 H = normalize(V + L);
        float distance = length(u_LightingData.PointLights[i].Position.xyz - fragPos);
        float attenuation = 1.0 / (distance * distance) + 0.0001;
        vec3 radiance = u_LightingData.PointLights[i].Color.xyz * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        vec3 kS = F; // coefficient Specular equal fresnel
        // Energy conservation: the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

        color += ambient + Lo;
    }

    //ambient*=calcAO;
    for(int i = 0; i < MAX_DIR_LIGHTS; ++i)
    {
        // reflectance equation
        vec3 Lo = vec3(0.0);
        // calculate per-light radiance
        vec3 L = normalize(-u_LightingData.DirLights[i].Direction);
        vec3 H = normalize(V + L);
        vec3 radiance = u_LightingData.DirLights[i].Color.xyz;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);       
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);
        const float I = NdotL *  u_LightingData.DirLights[i].Intensity;

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * I;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
        
        color += ambient + Lo;
    }

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0 / u_LightingData.Gamma)); 

    out_FragColor = vec4(color, 1.0);
}