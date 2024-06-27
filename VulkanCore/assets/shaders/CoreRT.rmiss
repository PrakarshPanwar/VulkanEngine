#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_control_flow_attributes : require

#include "Utils/Payload.glslh"
#include "Utils/Random.glslh"
#include "Utils/Math.glslh"
#include "Utils/Sampling.glslh"

layout(location = 0) rayPayloadInEXT RayPayload o_RayPayload;

struct PointLight
{
    vec4 Position;
	vec4 Color;
    float Radius;
    float Falloff;
};

layout(set = 0, binding = 4) uniform PointLightData
{
	int Count;
	PointLight PointLights[10];
} u_PointLight;

layout(set = 2, binding = 0) uniform samplerCube u_PrefilteredMap;

layout(set = 2, binding = 3) uniform SkyboxData
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

#include "Utils/Lighting.glslh"

void main()
{
	o_RayPayload.FFNormal = vec3(0.0);
	o_RayPayload.WorldPosition = vec3(0.0);
	o_RayPayload.Distance = -1.0;

//	LightSample lightSample;
//	if (IntersectsEmitter(lightSample, INFINITY))
//	{
//		vec3 Le = SampleEmitter(lightSample, o_RayPayload.BSDF);
//		o_RayPayload.Radiance += Le * o_RayPayload.Beta;
//		return;
//	}

	o_RayPayload.Radiance += texture(u_PrefilteredMap, gl_WorldRayDirectionEXT).xyz * o_RayPayload.Beta * u_Skybox.Intensity;
}
