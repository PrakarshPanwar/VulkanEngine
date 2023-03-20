#version 460 core

layout(location = 0) in vec2 v_FragOffset;
layout(location = 1) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Color;

layout(push_constant) uniform PointLight
{
	vec4 Position;
	vec4 Color;
} u_PointLight;

layout(binding = 1) uniform sampler2D u_PointLightTexture;

void main()
{
	o_Color = texture(u_PointLightTexture, v_TexCoord);
}