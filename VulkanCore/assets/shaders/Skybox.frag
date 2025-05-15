#version 460

layout(location = 0) out vec4 o_Color;

layout(binding = 1) uniform samplerCube u_CubeMap;
layout(location = 0) in vec3 v_TexCoord;

layout(push_constant) uniform SkyboxData
{
	float Intensity;
	float LOD;
} u_Skybox;

void main()
{
	vec3 color = textureLod(u_CubeMap, v_TexCoord, u_Skybox.LOD).rgb;
	color *= u_Skybox.Intensity;

	o_Color = vec4(color, 1.0);
}