#version 460 core
#extension GL_EXT_ray_tracing : require

struct RayPayload
{
	vec3 Color;
	vec4 ScatterDirection;
	float Distance;
	uint Seed;
};

layout(location = 0) rayPayloadInEXT RayPayload o_RayPayload;

layout(set = 2, binding = 3) uniform samplerCube u_CubeMap;

layout(set = 2, binding = 4) uniform SkyboxData
{
	float Intensity;
	float LOD;
} u_Skybox;

layout(push_constant) uniform RTSettings
{
	uint SampleCount;
	uint Bounces;
	uint RandomSeed;
	uint FrameIndex;
} u_Settings;

void main()
{
	vec3 color = textureLod(u_CubeMap, gl_WorldRayDirectionEXT, u_Skybox.LOD).rgb;
	color *= u_Skybox.Intensity / float(u_Settings.SampleCount);

	o_RayPayload.Color = color;
	o_RayPayload.Distance = -1.0;
}