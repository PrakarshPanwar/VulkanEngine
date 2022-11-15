#version 460 core

layout(location = 0) in vec2 v_FragOffset;
layout(location = 0) out vec4 o_Color;

layout(push_constant) uniform PointLight
{
	vec4 position;
	vec4 color;
	float radius;
} u_PointLight;

const float M_PI = 3.1415926538;

void main()
{
	float dis = sqrt(dot(v_FragOffset, v_FragOffset));
	if (dis >= 1.0)
		discard;

	float cosDis = 0.5 * (cos(dis * M_PI) + 1.0);
	o_Color = vec4(u_PointLight.color.xyz + cosDis, cosDis);
}