#version 460

layout(location = 0) in vec3 in_Pos;

layout(push_constant) uniform LightSpaceUBO
{
	mat4 Model;
	mat4 LightSpaceProjection;
} u_LightSpaceUBO;

void main()
{
	gl_Position = u_LightSpaceUBO.LightSpaceProjection * u_LightSpaceUBO.Model * vec4(in_Pos, 1.0f);
}
