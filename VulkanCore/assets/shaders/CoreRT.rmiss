#version 460 core
#extension GL_EXT_ray_tracing : require

struct RayPayload
{
	vec3 Color;
};

layout(location = 0) rayPayloadInEXT RayPayload o_RayPayload;

layout(binding = 5) uniform samplerCube u_CubeMap;

layout(binding = 6) uniform SkyboxData
{
	float Intensity;
	float LOD;
} u_Skybox;

void main()
{
	vec3 color = textureLod(u_CubeMap, gl_WorldRayDirectionEXT, 0.0).rgb;
	o_RayPayload.Color = color;
}
