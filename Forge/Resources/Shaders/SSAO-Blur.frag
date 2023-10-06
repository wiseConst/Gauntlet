#version 460

layout(location = 0) out float OutFragColor;
  
layout(location = 0) in vec2 InTexCoord;
  
layout(set = 0, binding = 0) uniform sampler2D u_SSAOMap;

void main() {
    const vec2 texelSize = 1.0 / vec2(textureSize(u_SSAOMap, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(u_SSAOMap, InTexCoord + offset).r;
        }
    }
    OutFragColor = result / (4.0 * 4.0);
}  