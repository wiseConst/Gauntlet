#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec2 in_UV;

layout(location = 0) out float out_FragColor;

layout(set = 0, binding = 0) uniform sampler2D u_PositionMap;
layout(set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout(set = 0, binding = 2) uniform sampler2D u_TexNoiseMap;

layout(constant_id = 0) const int NUM_SAMPLES = 16;
layout(set = 0, binding = 3) uniform UBSSAO
{
	mat4 CameraProjection;
	mat4 InvViewMatrix;
	vec4 Samples[NUM_SAMPLES];
	float Radius;
	float Bias;
	int Magnitude;
} u_UBSSAO;

vec2 GetNoiseUV()
{
	const ivec2 texDim   = textureSize(u_PositionMap, 0);
	const ivec2 noiseDim = textureSize(u_TexNoiseMap, 0);
	return vec2(float(texDim.x) / float(noiseDim.x), float(texDim.y) / float(noiseDim.y)) * in_UV;
}

void main()
{
	const vec3 worldPos = texture(u_PositionMap, in_UV).xyz;
	const vec3 worldN = normalize(texture(u_NormalMap, in_UV)).xyz;

	// Random rotation vector along Z axis in tangent space
	const vec3 randomVec = normalize(texture(u_TexNoiseMap, GetNoiseUV()).xyz);

	// TBN change-of-basis from tanget space -> world space (since normal in world)
	const vec3 T = normalize(randomVec - worldN * dot(randomVec, worldN)); // gram-schmidt orthogonalization(projecting randomVec onto worldN and making it perpendicular)
	const vec3 B = cross(worldN, T);
	const mat3 TBN = mat3(T, B, worldN);

	float occlusion = 0.0f;
	for(int i = 0; i < NUM_SAMPLES; ++i)
	{
	    vec3 samplePos = TBN * u_UBSSAO.Samples[i].xyz;
	    samplePos = worldPos + samplePos * u_UBSSAO.Radius;

		const vec3 sampleDir = normalize(samplePos - worldPos); // world space sample direction
        const float NdotS = max(dot(worldN, sampleDir), 0); // to make sure that the angle between normal and sample direction is not obtuse(>90 degrees)
		samplePos = vec3(u_UBSSAO.InvViewMatrix * vec4(samplePos, 1.0f)); // transform sample from world to view space

		vec4 offsetUV = vec4(samplePos, 1.0f);
		offsetUV      = u_UBSSAO.CameraProjection * offsetUV;   // view -> clip ([-w,w])
		offsetUV.xy = (offsetUV.xy / offsetUV.w) * 0.5f + 0.5f; // clip -> [-1, 1] (ndc) -> UV space
		offsetUV.y *= -1.0F;

		// To make sure that distance between sampled pos and init pos in a hemisphere radius range
		const vec4 worldOffsetPos = texture(u_PositionMap, offsetUV.xy);
		const float rangeCheck = smoothstep(0.0, 1.0, u_UBSSAO.Radius / abs(worldPos.z -  worldOffsetPos.z));
		
		const vec3 renderedFragmentViewPos = vec3(u_UBSSAO.InvViewMatrix * worldOffsetPos); // transform offset pos from world to view space
		occlusion += (renderedFragmentViewPos.z >= samplePos.z + u_UBSSAO.Bias ? 1.0f : 0.0f) * NdotS * rangeCheck; // in case rendered fragment is further than generated sample, then sample is not occluded
	}
	occlusion = 1.0f - (occlusion / (float(NUM_SAMPLES))); // subtract 1.0f(since 0 means occlusion, and 1 not) from mapped occlusion in range from [0, NUM_SAMPLES] to [0, 1]
	out_FragColor = pow(occlusion, u_UBSSAO.Magnitude); 
}