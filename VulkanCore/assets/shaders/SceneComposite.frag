#version 460 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in float v_Exposure;

layout(binding = 0) uniform sampler2D u_HDRTexture;

vec3 ReinhardTonemap(vec3 hdrColor, float exposure)
{
	vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
	return mapped;
}

void main()
{
	vec3 resColor = ReinhardTonemap(texture(u_HDRTexture, v_TexCoord).rgb, v_Exposure);
	o_Color = vec4(resColor, 1.0);
}
