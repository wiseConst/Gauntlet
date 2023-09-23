#version 460

layout(location = 0) in vec3 InPos;

layout(push_constant) uniform LightSpaceUBO
{
	mat4 Model;
	mat4 LightSpaceProjection;
} u_LightSpaceUBO;

void main()
{
	gl_Position = u_LightSpaceUBO.LightSpaceProjection * u_LightSpaceUBO.Model * vec4(InPos, 1.0f);
}
