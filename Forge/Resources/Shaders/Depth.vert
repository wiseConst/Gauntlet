#version 460

layout(location = 0) in vec3 aPos;

layout(push_constant) uniform UBO
{
	mat4 Model;
	mat4 LightSpace;
} ubo;


void main()
{
	gl_Position = ubo.LightSpace * ubo.Model * vec4(aPos, 1.0f);
}
