#version 460 core

layout(location = 0) out vec4 o_Color;

layout(binding = 1) uniform samplerCube u_CubeMap;
layout(location = 0) in vec3 v_TexCoord;

void main()
{
	o_Color = texture(u_CubeMap, v_TexCoord);
}