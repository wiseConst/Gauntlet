#version 460

layout(location = 0) in vec3 aPos;

layout(push_constant) uniform LightSpaceUBO
{
	mat4 Model;
	mat4 LightSpaceProjection;
} u_LightSpaceUBO;

void main()
{
	gl_Position = u_LightSpaceUBO.LightSpaceProjection * u_LightSpaceUBO.Model * vec4(aPos, 1.0f);
}
