#version 460

layout(location = 0) in vec2 InTexCoord;

layout(location = 0) out float OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D u_PositionMap;
layout(set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout(set = 0, binding = 2) uniform sampler2D u_TexNoiseMap;

layout(set = 0, binding = 3) uniform UBSSAO
{
	mat4 CameraProjection;
	vec4 Samples[64];
	vec4 ViewportSizeNoiseFactor;
} u_UBSSAO;

void main()
{
	const vec3 fragPos    = texture(u_PositionMap, InTexCoord).xyz;
	const vec3 normal     = texture(u_NormalMap, InTexCoord).xyz;

	const vec2 noiseScale = vec2(
						u_UBSSAO.ViewportSizeNoiseFactor.x / u_UBSSAO.ViewportSizeNoiseFactor.z, 
							u_UBSSAO.ViewportSizeNoiseFactor.y / u_UBSSAO.ViewportSizeNoiseFactor.z);	
	const vec3 randomVec  = texture(u_TexNoiseMap, InTexCoord * noiseScale).xyz * 2.0 - 1.0;

	const vec3 tangent =  normalize(randomVec - normal * dot(randomVec, normal));
	const vec3 bitangent = cross(normal, tangent);
	const mat3 TBN       = mat3(tangent, bitangent, normal);

	float occlusion = 0.0f;
	const float radius = 0.5f;
	const float bias = 0.025f;
	for(int i = 0; i < 64; ++i)
	{
	    vec3 Sample = TBN * u_UBSSAO.Samples[i].xyz;
	    Sample = fragPos + Sample * radius; 
	
		vec4 offset = vec4(Sample, 1.0);
		offset      = u_UBSSAO.CameraProjection * offset; 
		offset.xyz /= offset.w;
		offset.xyz  = offset.xyz * 0.5 + 0.5;

		const float sampleDepth = texture(u_PositionMap, offset.xy).z;
		const float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= Sample.z + bias ? 1.0f : 0.0f) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / 64);
	OutFragColor = occlusion; 
}