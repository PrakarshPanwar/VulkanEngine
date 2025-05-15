#version 460
#include "Utils/Buffers.glslh"

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 1) uniform sampler2D u_InputTexture;
layout(binding = 2) uniform sampler2D u_DepthTexture;
layout(binding = 3) uniform sampler2D u_BloomTexture;
layout(binding = 4) uniform sampler2D u_BloomDirtTexture;

layout(push_constant) uniform SceneData
{
	float Exposure;
	float DirtIntensity;
	uint Fog;
	float FogStartDistance;
	float FogFallOffDistance;
} u_SceneParams;

vec3 ReinhardTonemap(vec3 hdrColor)
{
	vec3 mapped = vec3(1.0) - exp(-hdrColor);
	return mapped;
}

vec3 ACESTonemap(vec3 hdrColor)
{
	const float A = 2.51;
	const float B = 0.03;
	const float C = 2.43;
	const float D = 0.59;
	const float E = 0.14;
	return clamp((hdrColor * (A * hdrColor + B)) / (hdrColor * (C * hdrColor + D) + E), 0.0, 1.0);
}

// Polynomial approximation of EaryChow's AgX sigmoid curve.
// In Blender's implementation, numbers could go a little bit over 1.0, so it's best to ensure
// this behaves the same as Blender's with values up to 1.1. Input values cannot be lower than 0.
vec3 AGXDefaultContrastApprox(vec3 x)
{
	// Generated with Excel trendline
	// Input data: Generated using python sigmoid with EaryChow's configuration and 57 steps
	// 6th order, intercept of 0.0 to remove an operation and ensure intersection at 0.0
	vec3 x2 = x * x;
	vec3 x4 = x2 * x2;
	return -0.20687445 * x + 6.80888933 * x2 - 37.60519607 * x2 * x + 93.32681938 * x4 - 95.2780858 * x4 * x + 33.96372259 * x4 * x2;
}

const mat3 LINEAR_SRGB_TO_LINEAR_REC2020 = mat3(
		vec3(0.6274, 0.0691, 0.0164),
		vec3(0.3293, 0.9195, 0.0880),
		vec3(0.0433, 0.0113, 0.8956));

// This is an approximation and simplification of EaryChow's AgX implementation that is used by Blender
// This code is based off of the script that generates the AgX_Base_sRGB.cube LUT that Blender uses
// Source: https://github.com/EaryChow/AgX_LUT_Gen/blob/main/AgXBasesRGB.py
vec3 AGXTonemap(vec3 color)
{
	const mat3 agx_inset_matrix = mat3(
			0.856627153315983, 0.137318972929847, 0.11189821299995,
			0.0951212405381588, 0.761241990602591, 0.0767994186031903,
			0.0482516061458583, 0.101439036467562, 0.811302368396859);

	// Combined inverse AgX outset matrix and linear Rec 2020 to linear sRGB matrices
	const mat3 agx_outset_rec2020_to_srgb_matrix = mat3(
			1.9648846919172409596, -0.29937618452442253746, -0.16440106280678278299,
			-0.85594737466675834968, 1.3263980951083531115, -0.23819967517076844919,
			-0.10883731725048386702, -0.02702191058393112346, 1.4025007379775505276);

	// LOG2_MIN      = -10.0
	// LOG2_MAX      =  +6.5
	// MIDDLE_GRAY   =  0.18
	const float min_ev = -12.4739311883324; // log2(pow(2, LOG2_MIN) * MIDDLE_GRAY)
	const float max_ev = 4.02606881166759; // log2(pow(2, LOG2_MAX) * MIDDLE_GRAY)

	// Do AGX in rec2020 to match Blender.
	color = LINEAR_SRGB_TO_LINEAR_REC2020 * color;

	// Preventing negative values is required for the AgX inset matrix to behave correctly
	// This could also be done before the Rec. 2020 transform, allowing the transform to
	// be combined with the AgX inset matrix, but doing this causes a loss of color information
	// that could be correctly interpreted within the Rec. 2020 color space
	color = max(color, vec3(0.0));

	color = agx_inset_matrix * color;

	// Log2 space encoding.
	color = max(color, 1e-10); // Prevent log2(0.0). Possibly unnecessary.
	// Must be clamped because agx_blender_default_contrast_approx may not work well with values above 1.0
	color = clamp(log2(color), min_ev, max_ev);
	color = (color - min_ev) / (max_ev - min_ev);

	// Apply sigmoid function approximation
	color = AGXDefaultContrastApprox(color);

	// Convert back to linear before applying outset matrix.
	color = pow(color, vec3(2.4));

	// Apply outset to make the result more chroma-laden and then go back to linear sRGB
	color = agx_outset_rec2020_to_srgb_matrix * color;

	// Simply hard clip instead of Blender's complex lusRGB.compensate_low_side
	color = max(color, vec3(0.0));

	return color;
}

float LinearizeDepth(const float screenDepth)
{
	float depthLinearizeMul = u_Camera.DepthUnpackConsts.x;
	float depthLinearizeAdd = u_Camera.DepthUnpackConsts.y;
	return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

vec3 UpsampleTent9(sampler2D tex, float lod, vec2 uv, vec2 texelSize, float radius)
{
	vec4 offset = texelSize.xyxy * vec4(1.0, 1.0, -1.0, 0.0) * radius;
	vec3 result = textureLod(tex, uv, lod).rgb * 4.0;

	result += textureLod(tex, uv - offset.xy, lod).rgb;
	result += textureLod(tex, uv - offset.wy, lod).rgb * 2.0;
	result += textureLod(tex, uv - offset.zy, lod).rgb;
	result += textureLod(tex, uv + offset.zw, lod).rgb * 2.0;
	result += textureLod(tex, uv + offset.xw, lod).rgb * 2.0;
	result += textureLod(tex, uv + offset.zy, lod).rgb;
	result += textureLod(tex, uv + offset.wy, lod).rgb * 2.0;
	result += textureLod(tex, uv + offset.xy, lod).rgb;

	return result * (1.0 / 16.0);
}

void main()
{
	vec3 color = texture(u_InputTexture, v_TexCoord).rgb;

	ivec2 texSize = textureSize(u_BloomTexture, 0);
	vec2 fTexSize = vec2(float(texSize.x), float(texSize.y));
	vec3 bloom = UpsampleTent9(u_BloomTexture, 0, v_TexCoord, 1.0 / fTexSize, 0.5);
	vec3 bloomDirt = texture(u_BloomDirtTexture, v_TexCoord).rgb * u_SceneParams.DirtIntensity;

	if (bool(u_SceneParams.Fog))
	{
		float depth = texture(u_DepthTexture, v_TexCoord).r;
		depth = LinearizeDepth(depth);

		const float fogStartDistance = u_SceneParams.FogStartDistance;
		const float bloomFogStartDistance = fogStartDistance;
		const float fogFallOffDistance = u_SceneParams.FogFallOffDistance;

		const float fogAmount = smoothstep(fogStartDistance, fogStartDistance + fogFallOffDistance, depth);
		const float fogAmountBloom = smoothstep(bloomFogStartDistance, bloomFogStartDistance + fogFallOffDistance, depth);

		vec3 fogColor = vec3(0.11, 0.12, 0.15);
		fogColor *= 2.0;
		vec3 bloomClamped = clamp(bloom * (1.0 - fogAmountBloom), 0.0, 1.0);
		float intensity = (bloomClamped.r + bloomClamped.g + bloomClamped.b) / 3.0;
		fogColor = mix(fogColor, color, intensity);

		color = mix(color, fogColor, fogAmount);
		bloom *= (1.0 - fogAmountBloom);
	}

	color += bloom;
	color += bloom * bloomDirt;

	color *= u_SceneParams.Exposure;
    color = AGXTonemap(color);

	o_Color = vec4(color, 1.0);
}
