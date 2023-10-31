#version 460

layout(location = 0) in vec2 InTexCoord;

layout(location = 0) out vec4 OutFragColor;

layout(set = 0, binding = 0) uniform sampler2D u_FinalImage;

void main()
{
    // The technique works by mapping each fragment to the center of its closest, 
    // non-overlapping pixel-sized window. These windows are laid out in a grid 
    // over the input texture. The center-of-the-window fragments determine the color 
    // for the other fragments in their window.

    const int pixelSize = 4;
    float x = int(gl_FragCoord.x) % pixelSize;
    float y = int(gl_FragCoord.y) % pixelSize;

    x = floor(pixelSize / 2.0) - x;
    y = floor(pixelSize / 2.0) - y;

    x = gl_FragCoord.x + x;
    y = gl_FragCoord.y + y;

    const vec2 texSize = textureSize(u_FinalImage, 0);
    OutFragColor = texture(u_FinalImage, vec2(x, y) / texSize);
}