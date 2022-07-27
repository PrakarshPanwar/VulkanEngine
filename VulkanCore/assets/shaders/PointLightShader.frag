#version 460 core

layout(location = 0) in vec2 v_FragOffset;
layout(location = 0) out vec4 o_Color;

struct PointLight
{
	vec4 position;
	vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
	mat4 invView;
	vec4 ambientLightColor;
	PointLight pointLights[10];
	int numLights;
} ubo;

layout(push_constant) uniform Push
{
	vec4 position;
	vec4 color;
	float radius;
} push;

const float M_PI = 3.1415926538;

void main()
{
	float dis = sqrt(dot(v_FragOffset, v_FragOffset));
	if (dis >= 1.0)
		discard;

	float cosDis = 0.5 * (cos(dis * M_PI) + 1.0);
	o_Color = vec4(push.color.xyz + cosDis, cosDis);
}