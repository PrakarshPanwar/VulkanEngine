#version 460
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

// Skybox
layout(set = 2, binding = 0) uniform sampler2D u_HDRTexture;
layout(set = 2, binding = 1) uniform sampler2D u_PDFTexture;
layout(set = 2, binding = 2) uniform sampler2D u_CDFTexture;

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

float EnvironmentPDF()
{
	vec3 direction = gl_WorldRayDirectionEXT;
	float theta = acos(clamp(direction.y, -1.0, 1.0));
	vec2 uv = vec2((PI + atan(direction.z, direction.x)) * INV_2PI, theta * INV_PI);
	float pdf = texture(u_CDFTexture, uv).y * texture(u_PDFTexture, vec2(0.0, uv.y)).y;
	return (pdf * HDR_RESOLUTION) / (TWO_PI * PI * sin(theta));
}

void main()
{
	o_RayPayload.FFNormal = vec3(0.0);
	o_RayPayload.WorldPosition = vec3(0.0);
	o_RayPayload.Distance = -1.0;

	LightSample lightSample;
	if (IntersectsEmitter(lightSample, INFINITY))
	{
		vec3 Le = SampleEmitter(lightSample, o_RayPayload.BSDF);
		o_RayPayload.Radiance += Le * o_RayPayload.Beta;
		return;
	}

	float misWeight = 1.0;
	vec2 uv = vec2((PI + atan(gl_WorldRayDirectionEXT.z, gl_WorldRayDirectionEXT.x)) * INV_2PI, acos(gl_WorldRayDirectionEXT.y) * INV_PI);

	if (o_RayPayload.Depth > 0)
	{
		float lightPdf = EnvironmentPDF();
		misWeight = PowerHeuristic(o_RayPayload.BSDF.PDF, lightPdf);
	}

	o_RayPayload.Radiance += misWeight * texture(u_HDRTexture, uv).xyz * o_RayPayload.Beta * u_Skybox.Intensity;
}
