#version 460 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
	vec2 CameraTanHalfFOV;
} u_Camera;

layout(binding = 1) uniform sampler2D u_InputTexture;
layout(binding = 2) uniform sampler2D u_BloomTexture;
layout(binding = 3) uniform sampler2D u_BloomDirtTexture;

layout(push_constant) uniform SceneData
{
	float Exposure;
	float DirtIntensity;
} u_SceneParams;

layout(binding = 4) uniform sampler2D u_DepthTexture;

layout(binding = 5) uniform DOFData
{
	float FocusPoint;
	float FocusScale;
	float Near;
	float Far;
} u_DOF;

vec3 CalculateViewPosition(vec2 texCoord, float depth, vec2 fovScale)
{
	vec2 halfNDCPosition = vec2(0.5) - texCoord;
    vec3 viewPosition = vec3(halfNDCPosition * fovScale * -depth, -depth);
    return viewPosition;
}

const float GOLDEN_ANGLE = 2.39996323;
const float MAX_BLUR_SIZE = 20.0; 
const float RAD_SCALE = 0.5; // Smaller = nicer blur, larger = faster

float LinearizeDepth(const float screenDepth)
{
	return u_DOF.Far * u_DOF.Near / (u_DOF.Far - screenDepth * (u_DOF.Far - u_DOF.Near));
}

float GetBlurSize(float depth, float focusPoint, float focusScale)
{
	float coc = clamp((1.0 / focusPoint - 1.0 / depth) * focusScale, -1.0, 1.0);
	return abs(coc) * MAX_BLUR_SIZE;
}

vec3 DepthOfField(vec2 texCoord, float focusPoint, float focusScale, vec2 texelSize)
{
	float centerDepth = LinearizeDepth(texture(u_DepthTexture, texCoord).r);
	float centerSize = GetBlurSize(centerDepth, focusPoint, focusScale);
	vec3 color = texture(u_InputTexture, v_TexCoord).rgb;
	float tot = 1.0;
	float radius = RAD_SCALE;
	for (float ang = 0.0; radius < MAX_BLUR_SIZE; ang += GOLDEN_ANGLE)
	{
		vec2 tc = texCoord + vec2(cos(ang), sin(ang)) * texelSize * radius;
		vec3 sampleColor = texture(u_InputTexture, tc).rgb;
		float sampleDepth = LinearizeDepth(texture(u_DepthTexture, tc).r);
		float sampleSize = GetBlurSize(sampleDepth, focusPoint, focusScale);
		if (sampleDepth > centerDepth)
			sampleSize = clamp(sampleSize, 0.0, centerSize * 2.0);
		float m = smoothstep(radius - 0.5, radius + 0.5, sampleSize);
		color += mix(color / tot, sampleColor, m);
		tot += 1.0;
		radius += RAD_SCALE / radius;
	}

	return color /= tot;
}

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
	vec3 color = texture(u_InputTexture, v_TexCoord).rgb;
	ivec2 texSize = textureSize(u_InputTexture, 0);
	vec2 fTexSize = vec2(float(texSize.x), float(texSize.y));

	// Bloom
//	vec3 bloom = UpsampleTent9(u_BloomTexture, 0, v_TexCoord, 1.0 / fTexSize, 0.5);
//	vec3 bloomDirt = texture(u_BloomDirtTexture, v_TexCoord).rgb * u_SceneParams.DirtIntensity;
//	color += bloom;
//	color += bloom * bloomDirt;
//
//	// Depth of Field
	vec2 uv = v_TexCoord;
	vec3 dofColor = DepthOfField(uv, u_DOF.FocusPoint, u_DOF.FocusScale, 1.0 / fTexSize);
	vec2 vuv = uv - vec2(0.5);

	vuv.x *= float(texSize.x) / float(texSize.y);
    float vignette = pow(1.0 - length(vuv * vec2(1.0, 1.3)), 1.0);
    color = mix(vec4(dofColor, 1.0), vec4(dofColor, 1.0) * vignette, 0.75).rgb;

	color *= u_SceneParams.Exposure;
    color = ACESTonemap(color);

	o_Color = vec4(color, 1.0);
}
