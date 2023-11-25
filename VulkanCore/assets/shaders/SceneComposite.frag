#version 460 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
	vec2 DepthUnpackConsts;
} u_Camera;

layout(binding = 1) uniform sampler2D u_InputTexture;
layout(binding = 2) uniform sampler2D u_DepthTexture;
layout(binding = 3) uniform sampler2D u_BloomTexture;
layout(binding = 4) uniform sampler2D u_BloomDirtTexture;

layout(push_constant) uniform SceneData
{
	float Exposure;
	float DirtIntensity;
	uint EnableFog;
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

	if (u_SceneParams.EnableFog == 1)
	{
		float depth = texture(u_DepthTexture, v_TexCoord).r;
		depth = LinearizeDepth(depth);

		//const float fogStartDistance = 5.5;
		const float bloomFogStartDistance = 5.0;
		const float fogFallOffDistance = 30.0;

		//const float fogAmount = smoothstep(fogStartDistance, fogStartDistance + fogFallOffDistance, depth);
		const float fogAmountBloom = smoothstep(bloomFogStartDistance, bloomFogStartDistance + fogFallOffDistance, depth);

		vec3 fogColor = vec3(0.11, 0.12, 0.15);
		fogColor *= 10.0;
		vec3 bloomClamped = clamp(bloom * (1.0 - fogAmountBloom), 0.0, 1.0);
		float intensity = (bloomClamped.r + bloomClamped.g + bloomClamped.b) / 3.0;
		fogColor = mix(fogColor, color, intensity);

		color = mix(color, fogColor, intensity);
		bloom *= (1.0 - fogAmountBloom);
	}

	color += bloom;
	color += bloom * bloomDirt;

	color *= u_SceneParams.Exposure;
    color = ACESTonemap(color);

	o_Color = vec4(color, 1.0);
}
