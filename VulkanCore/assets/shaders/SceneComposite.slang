// Shader File: "SceneComposite.slang"

[shader("vertex")]
void VSMain(in uint id : SV_VertexID, out float4 position : SV_Position, out float2 texCoord : TEXCOORD0)
{
    texCoord = float2((id << 1) & 2, id & 2);
    position = float4(texCoord * 2.0f - 1.0f, 0, 1);
} // End of Vertex Stage

import Buffers;

struct SceneData
{
    float Exposure;
    float DirtIntensity;
    uint Fog;
    float FogStartDistance;
    float FogFallOffDistance;
};

[[vk::push_constant]] ConstantBuffer<SceneData> c_SceneParams;

[[vk::binding(0, 0)]] ConstantBuffer<Camera> c_Camera;
[[vk::binding(1, 0)]] Sampler2D r_InputTexture;
[[vk::binding(2, 0)]] Sampler2D r_DepthTexture;
[[vk::binding(3, 0)]] Sampler2D r_BloomTexture;
[[vk::binding(4, 0)]] Sampler2D r_BloomDirtTexture;

// Polynomial approximation of EaryChow's AgX sigmoid curve.
// In Blender's implementation, numbers could go a little bit over 1.0, so it's best to ensure
// this behaves the same as Blender's with values up to 1.1. Input values cannot be lower than 0.
float3 AGXDefaultContrastApprox(float3 x)
{
    // Generated with Excel trendline
    // Input data: Generated using python sigmoid with EaryChow's configuration and 57 steps
    // 6th order, intercept of 0.0 to remove an operation and ensure intersection at 0.0
    float3 x2 = x * x;
    float3 x4 = x2 * x2;
    return -0.20687445f * x + 6.80888933f * x2 - 37.60519607f * x2 * x + 93.32681938f * x4 - 95.2780858f * x4 * x + 33.96372259f * x4 * x2;
}

static const float3x3 LINEAR_SRGB_TO_LINEAR_REC2020 = float3x3(
		float3(0.6274f, 0.0691f, 0.0164f),
		float3(0.3293f, 0.9195f, 0.0880f),
		float3(0.0433f, 0.0113f, 0.8956f)
);

// This is an approximation and simplification of EaryChow's AgX implementation that is used by Blender
// This code is based off of the script that generates the AgX_Base_sRGB.cube LUT that Blender uses
// Source: https://github.com/EaryChow/AgX_LUT_Gen/blob/main/AgXBasesRGB.py
float3 AGXTonemap(float3 color)
{
    const float3x3 agx_inset_matrix = float3x3(
			0.856627153315983f, 0.137318972929847f, 0.11189821299995f,
			0.0951212405381588f, 0.761241990602591f, 0.0767994186031903f,
			0.0482516061458583f, 0.101439036467562f, 0.811302368396859f);

    // Combined inverse AgX outset matrix and linear Rec 2020 to linear sRGB matrices
    const float3x3 agx_outset_rec2020_to_srgb_matrix = float3x3(
			1.9648846919172409596f, -0.29937618452442253746f, -0.16440106280678278299f,
			-0.85594737466675834968f, 1.3263980951083531115f, -0.23819967517076844919f,
			-0.10883731725048386702f, -0.02702191058393112346f, 1.4025007379775505276f);

    // LOG2_MIN      = -10.0
    // LOG2_MAX      =  +6.5
    // MIDDLE_GRAY   =  0.18
    const float min_ev = -12.4739311883324f; // log2(pow(2, LOG2_MIN) * MIDDLE_GRAY)
    const float max_ev = 4.02606881166759f;  // log2(pow(2, LOG2_MAX) * MIDDLE_GRAY)

    // Do AGX in rec2020 to match Blender.
    color = mul(color, LINEAR_SRGB_TO_LINEAR_REC2020);

    // Preventing negative values is required for the AgX inset matrix to behave correctly
    // This could also be done before the Rec. 2020 transform, allowing the transform to
    // be combined with the AgX inset matrix, but doing this causes a loss of color information
    // that could be correctly interpreted within the Rec. 2020 color space
    color = max(color, 0.0f);

    color = mul(color, agx_inset_matrix);

    // Log2 space encoding.
    color = max(color, 1e-10); // Prevent log2(0.0). Possibly unnecessary.
    // Must be clamped because agx_blender_default_contrast_approx may not work well with values above 1.0
    color = clamp(log2(color), min_ev, max_ev);
    color = (color - min_ev) / (max_ev - min_ev);

    // Apply sigmoid function approximation
    color = AGXDefaultContrastApprox(color);

    // Convert back to linear before applying outset matrix.
    color = pow(color, 2.4f);

    // Apply outset to make the result more chroma-laden and then go back to linear sRGB
    color = mul(color, agx_outset_rec2020_to_srgb_matrix);

    // Simply hard clip instead of Blender's complex lusRGB.compensate_low_side
    color = max(color, 0.0f);

    return color;
}

float LinearizeDepth(const float screenDepth)
{
    float depthLinearizeMul = c_Camera.DepthUnpackConsts.x;
    float depthLinearizeAdd = c_Camera.DepthUnpackConsts.y;
    return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

float3 UpsampleTent9(Sampler2D tex, float lod, float2 uv, float2 texelSize, float radius)
{
    float4 offset = texelSize.xyxy * float4(1.0f, 1.0f, -1.0f, 0.0f) * radius;

    // Center
    float3 result = tex.SampleLevel(uv, lod).rgb * 4.0f;

    result += tex.SampleLevel(uv - offset.xy, lod).rgb;
    result += tex.SampleLevel(uv - offset.wy, lod).rgb * 2.0f;
    result += tex.SampleLevel(uv - offset.zy, lod).rgb;

    result += tex.SampleLevel(uv + offset.zw, lod).rgb * 2.0f;
    result += tex.SampleLevel(uv + offset.xw, lod).rgb * 2.0f;

    result += tex.SampleLevel(uv + offset.zy, lod).rgb;
    result += tex.SampleLevel(uv + offset.wy, lod).rgb * 2.0f;
    result += tex.SampleLevel(uv + offset.xy, lod).rgb;

    return result * (1.0f / 16.0f);
}

[shader("fragment")]
void FSMain(in float2 texCoord : TEXCOORD0, out float4 o_Color : SV_Target)
{
    float3 color = r_InputTexture.Sample(texCoord).rgb;

    float texWidth, texHeight;
    r_BloomTexture.GetDimensions(texWidth, texHeight);

    float2 texSize = float2(texWidth, texHeight);
    float3 bloom = UpsampleTent9(r_BloomTexture, 0, texCoord, 1.0f / texSize, 0.5f);
    float3 bloomDirt = r_BloomDirtTexture.Sample(texCoord).rgb * c_SceneParams.DirtIntensity;

    if (bool(c_SceneParams.Fog))
    {
        float depth = r_DepthTexture.Sample(texCoord).r;
        depth = LinearizeDepth(depth);

        const float fogStartDistance = c_SceneParams.FogStartDistance;
        const float bloomFogStartDistance = fogStartDistance;
        const float fogFallOffDistance = c_SceneParams.FogFallOffDistance;

        const float fogAmount = smoothstep(fogStartDistance, fogStartDistance + fogFallOffDistance, depth);
        const float fogAmountBloom = smoothstep(bloomFogStartDistance, bloomFogStartDistance + fogFallOffDistance, depth);

        float3 fogColor = float3(0.11f, 0.12f, 0.15f);
        fogColor *= 2.0f;
        float3 bloomClamped = saturate(bloom * (1.0f - fogAmountBloom));
        float intensity = (bloomClamped.r + bloomClamped.g + bloomClamped.b) / 3.0f;
        fogColor = lerp(fogColor, color, intensity);

        color = lerp(color, fogColor, fogAmount);
        bloom *= (1.0f - fogAmountBloom);
    }

    color += bloom;
    color += bloom * bloomDirt;

    color *= c_SceneParams.Exposure;
    color = AGXTonemap(color);

    o_Color = float4(color, 1.0f);
}
