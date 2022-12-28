#version 460 core

layout(location = 0) out vec4 o_Color;

layout(binding = 1) uniform samplerCube u_CubeMap;
layout(location = 0) in vec3 v_TexCoord;

layout(push_constant) uniform LevelOfDetail
{
	float LOD;
} u_LevelOfDetail;

void main()
{
	vec3 color = textureLod(u_CubeMap, v_TexCoord, u_LevelOfDetail.LOD).rgb;
	o_Color = vec4(color, 1.0);
}