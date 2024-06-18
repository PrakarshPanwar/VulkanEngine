#version 460 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
	vec2 DepthUnpackConsts;
	vec2 CameraTanHalfFOV;
} u_Camera;

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
	vec2 texCoord = vec2(v_TexCoord.x, 1.0 - v_TexCoord.y);

	vec3 color = texture(u_InputTexture, texCoord).rgb;

	ivec2 texSize = textureSize(u_BloomTexture, 0);
	vec2 fTexSize = vec2(float(texSize.x), float(texSize.y));
	vec3 bloom = UpsampleTent9(u_BloomTexture, 0, texCoord, 1.0 / fTexSize, 0.5);
	vec3 bloomDirt = texture(u_BloomDirtTexture, texCoord).rgb * u_SceneParams.DirtIntensity;

	if (bool(u_SceneParams.Fog))
	{
		float depth = texture(u_DepthTexture, texCoord).r;
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

	o_Color = vec4(color, 1.0);
}
