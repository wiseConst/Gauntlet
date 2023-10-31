#version 460

layout(location = 0) in vec2 InTexCoord;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D u_FinalImage;

void main()
{
    const vec2 r_displacement = vec2(3.0, 0.0);
    const vec2 g_displacement = vec2(0.0, 0.0);
    const vec2 b_displacement = vec2(-3.0, 0.0);
    const vec2 pixelSize = vec2(1.0) / textureSize(u_FinalImage, 0);

    const float r = texture(u_FinalImage, InTexCoord + pixelSize * r_displacement).r;
    const float g = texture(u_FinalImage, InTexCoord + pixelSize * g_displacement).g;
    const float b = texture(u_FinalImage, InTexCoord + pixelSize * b_displacement).b;
    
    OutFragColor = vec4(r, g, b, 1.0);
}