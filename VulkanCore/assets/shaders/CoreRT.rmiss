#version 460 core
#extension GL_EXT_ray_tracing : require

struct RayPayload
{
	vec3 Color;
};

layout(location = 0) rayPayloadInEXT RayPayload o_RayPayload;

layout(set = 2, binding = 3) uniform samplerCube u_CubeMap;

layout(set = 2, binding = 4) uniform SkyboxData
{
	float Intensity;
	float LOD;
} u_Skybox;

void main()
{
	vec3 color = textureLod(u_CubeMap, gl_WorldRayDirectionEXT, u_Skybox.LOD).rgb;
	color *= u_Skybox.Intensity;

	o_RayPayload.Color = color;
}
