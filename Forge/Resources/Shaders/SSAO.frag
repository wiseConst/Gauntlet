#version 460

layout(location = 0) in vec2 InTexCoord;

layout(location = 0) out float OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D PositionMap;
layout(set = 0, binding = 1) uniform sampler2D NormalMap;
layout(set = 0, binding = 2) uniform sampler2D TexNoiseMap;

layout(set = 0, binding = 3) uniform UBSSAO
{
	vec3 Samples[64];
	mat4 CameraProjection;
	vec2 ViewportSize;
	float NoiseFactor;
} InUBSSAO;

void main()
{
	const vec2 noiseScale = vec2(InUBSSAO.ViewportSize.x / InUBSSAO.NoiseFactor, InUBSSAO.ViewportSize.y / InUBSSAO.NoiseFactor);
	const vec3 fragPos    = texture(PositionMap, InTexCoord).xyz;
	const vec3 normal     = texture(NormalMap, InTexCoord).xyz;
	const vec3 randomVec  = texture(TexNoiseMap, InTexCoord * noiseScale).xyz;
	
	const vec3 tangent =  normalize(randomVec - normal * dot(randomVec, normal));
	const vec3 bitangent = cross(normal, tangent);
	const mat3 TBN       = mat3(tangent, bitangent, normal);

	float occlusion = 0.0f;
	const float radius = 0.5f;
	const float bias = 0.025f;
	for(int i = 0; i < 64; ++i)
	{
	    vec3 Sample = TBN * InUBSSAO.Samples[i];
	    Sample = fragPos + Sample * radius; 
	
		vec4 offset = vec4(Sample, 1.0);
		offset      = InUBSSAO.CameraProjection * offset;    // переход из видового  клиповое
		offset.xyz /= offset.w;               // перспективное деление 
		offset.xyz  = offset.xyz * 0.5 + 0.5; // преобразование к интервалу [0., 1.] 

		const float sampleDepth = texture(PositionMap, offset.xy).z;
		occlusion += (sampleDepth >= Sample.z + bias ? 1.0 : 0.0);
	}
	occlusion = 1.0f - (occlusion / 64);
	OutFragColor = occlusion; 
}