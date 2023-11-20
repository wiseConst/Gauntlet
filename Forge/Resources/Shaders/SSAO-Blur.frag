#version 460

layout(location = 0) out float out_FragColor;
  
layout(location = 0) in vec2 in_UV;

layout(constant_id = 0) const int blurRange = 2;
layout(set = 0, binding = 0) uniform sampler2D u_SSAOMap;

void main() {
    const vec2 texelSize = 1.0 / vec2(textureSize(u_SSAOMap, 0));
    int n = 0;
    float result = 0.0;
    for (int x = -blurRange; x < blurRange; ++x) 
    {
        for (int y = -blurRange; y < blurRange; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(u_SSAOMap, in_UV + offset).r;
            ++n;
        }
    }
    out_FragColor = result / (float(n));
}  