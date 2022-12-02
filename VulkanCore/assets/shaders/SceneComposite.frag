#version 460 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in float v_Exposure;

layout(binding = 0) uniform sampler2D u_InputTexture;

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
	vec3 rColor = texture(u_InputTexture, v_TexCoord).rgb;

	ivec2 texSize = textureSize(u_InputTexture, 0);
	vec2 fTexSize = vec2(float(texSize.x), float(texSize.y));
	rColor += UpsampleTent9(u_InputTexture, 0, v_TexCoord, 1.0 / fTexSize, 0.5);

	rColor *= v_Exposure;
    rColor = ACESTonemap(rColor);

	o_Color = vec4(rColor, 1.0);
}
