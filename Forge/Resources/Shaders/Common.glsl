#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_separate_shader_objects : enable

struct DirectionalLight
{
    vec3 Color;
    float Intensity;
    vec3 Direction;
    int CastShadows;
};

struct PointLight
{
    vec4 Position;
    vec4 Color;
    float Ambient;    // useless
    float Specular;   // useless
    float Shininess;  // useless
    int IsActive;
};

struct SpotLight
{
    vec4 Position;
    vec4 Color;
    vec3 Direction;
    float CutOff;

    float Ambient;  // useless
    float Specular;
    float Shininess;  // useless
    int Active;
};

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8
#define MAX_DIR_LIGHTS 4