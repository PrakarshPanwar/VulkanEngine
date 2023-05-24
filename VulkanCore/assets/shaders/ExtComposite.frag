#version 460 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 1) uniform sampler2D u_InputTexture;
layout(binding = 2) uniform sampler2D u_BloomTexture;
layout(binding = 3) uniform sampler2D u_BloomDirtTexture;

layout(push_constant) uniform SceneData
{
	float Exposure;
	float DirtIntensity;
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
	color += bloom;
	color += bloom * bloomDirt;

	o_Color = vec4(color, 1.0);
}
