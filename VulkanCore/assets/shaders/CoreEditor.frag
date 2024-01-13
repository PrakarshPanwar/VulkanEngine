#version 460 core

layout(location = 0) out int o_EntityID;

layout(location = 0) in flat int o_InstanceID;

struct PointLight
{
    vec4 Position;
	vec4 Color;
    float Radius;
    float Falloff;
};

struct SpotLight
{
    vec4 Position;
    vec4 Color;
    vec3 Direction;
    float InnerCutoff;
    float OuterCutoff;
    float Radius;
    float Falloff;
};

// Buffer Data
layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
    vec2 DepthUnpackConsts;
} u_Camera;

layout(set = 0, binding = 1) uniform PointLightData
{
	int Count;
	PointLight PointLights[10];
} u_PointLight;

layout(set = 0, binding = 2) uniform SpotLightData
{
    int Count;
    SpotLight SpotLights[10];
} u_SpotLight;

void main()
{
    o_EntityID = o_InstanceID;
}