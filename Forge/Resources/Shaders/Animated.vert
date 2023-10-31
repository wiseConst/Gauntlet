#version 460

layout(location = 0) in ivec4 InBoneIDs; 
layout(location = 1) in vec3 InPosition;
layout(location = 2) in vec4 InWeights;
layout(location = 3) in vec2 InTexCoord;
layout(location = 4) in vec3 InNormal;
layout(location = 5) in vec3 InTangent;
	
layout(location = 0) out vec2 OutTexCoord;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;	

layout(set = 0, binding = 0) uniform AnimationData
{
    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 finalBonesMatrices[MAX_BONES];
} u_AnimationData;

void main()
{
    vec4 totalPosition = vec4(0.0f);

    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; ++i)
    {
        if(InBoneIDs[i] == -1) continue;

        if(InBoneIDs[i] >= MAX_BONES) 
        {
            totalPosition = vec4(InPosition,1.0f);
            break;
        }

        const vec4 localPosition = u_AnimationData.finalBonesMatrices[InBoneIDs[i]] * vec4(InPosition, 1.0f);

        totalPosition += localPosition * InWeights[i];
       // vec3 localNormal = mat3(u_AnimationData.finalBonesMatrices[InBoneIDs[i]]) * InNormal;
    }
		
    const mat4 viewModel = u_AnimationData.view * u_AnimationData.model;
    gl_Position = u_AnimationData.projection * viewModel * totalPosition;
    OutTexCoord = InTexCoord;
}