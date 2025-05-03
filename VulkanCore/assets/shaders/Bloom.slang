// Shader File : "Bloom.slang"

[[vk::binding(0)]] RWTexture2D o_Image;

[[vk::binding(1)]] Sampler2D r_Texture;
[[vk::binding(2)]] Sampler2D r_BloomTexture;

struct BloomParams
{
    float2 LodAndMode;
    float Threshold;
    float Knee;
};

[[vk::push_constant]] ConstantBuffer<BloomParams> c_Params;

enum BloomMode
{
    Prefilter = 0,
    Downsample,
    UpsampleFirst,
    Upsample
};

static const float Epsilon = 1.0e-4;

float3 DownsampleBox13(Sampler2D tex, float lod, float2 uv, float2 texelSize)
{
    // Center
    float3 A = tex.SampleLevel(uv, lod).rgb;

    texelSize *= 0.5; // Sample from center of texels

    // Inner Box
    float3 B = tex.SampleLevel(uv + texelSize * float2(-1.0f, -1.0f), lod).rgb;
    float3 C = tex.SampleLevel(uv + texelSize * float2(-1.0f, 1.0f), lod).rgb;
    float3 D = tex.SampleLevel(uv + texelSize * float2(1.0f, 1.0f), lod).rgb;
    float3 E = tex.SampleLevel(uv + texelSize * float2(1.0f, -1.0f), lod).rgb;

    // Outer Box
    float3 F = tex.SampleLevel(uv + texelSize * float2(-2.0f, -2.0f), lod).rgb;
    float3 G = tex.SampleLevel(uv + texelSize * float2(-2.0f, 0.0f), lod).rgb;
    float3 H = tex.SampleLevel(uv + texelSize * float2(0.0f, 2.0f), lod).rgb;
    float3 I = tex.SampleLevel(uv + texelSize * float2(2.0f, 2.0f), lod).rgb;
    float3 J = tex.SampleLevel(uv + texelSize * float2(2.0f, 2.0f), lod).rgb;
    float3 K = tex.SampleLevel(uv + texelSize * float2(2.0f, 0.0f), lod).rgb;
    float3 L = tex.SampleLevel(uv + texelSize * float2(-2.0f, -2.0f), lod).rgb;
    float3 M = tex.SampleLevel(uv + texelSize * float2(0.0f, -2.0f), lod).rgb;

    // Weights
    float3 result = 0.0f;
    // Inner Box
    result += (B + C + D + E) * 0.5f;
    // Bottom-left Box
    result += (F + G + A + M) * 0.125f;
    // Top-left Box
    result += (G + H + I + A) * 0.125f;
    // Top-right Box
    result += (A + I + J + K) * 0.125f;
    // Bottom-right Box
    result += (M + A + K + L) * 0.125f;

    result *= 0.25f;

    return result;
}

float4 QuadraticThreshold(float4 color, float threshold, float3 curve)
{
    // Maximum pixel brightness
    float brightness = max(max(color.r, color.g), color.b);
    // Quadratic Curve
    float rq = clamp(brightness - curve.x, 0.0f, curve.y);
    rq = (rq * rq) * curve.z;
    color *= max(rq, brightness - threshold) / max(brightness, Epsilon);
    return color;
}

float4 Prefilter(float4 color, float2 uv)
{
    float3 pr = float3(c_Params.Threshold - c_Params.Knee, c_Params.Knee * 2.0f, 0.25f / c_Params.Knee);
    float clampValue = 20.0f;
    color = min(clampValue, color);
    color = QuadraticThreshold(color, c_Params.Threshold, pr);
    return color;
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

[shader("compute")]
[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID)
{
    float imgWidth, imgHeight;
    o_Image.GetDimensions(imgWidth, imgHeight);

    float2 imgSize = float2(imgWidth, imgHeight);
    float2 texCoords = (threadID.xy + 0.5f) / imgSize;

    float texWidth, texHeight, texMips;
    r_Texture.GetDimensions(int(c_Params.LodAndMode.x), texWidth, texHeight, texMips);
    float2 texSize = float2(texWidth, texHeight);

    float4 color = float4(1.0f, 0.0f, 1.0f, 1.0f);
    BloomMode bloomMode = (BloomMode)c_Params.LodAndMode.y;

    switch (bloomMode)
    {
    case BloomMode::Prefilter:
    {
        color.rgb = DownsampleBox13(r_Texture, 0, texCoords, 1.0f / texSize);
        color = Prefilter(color, texCoords);
        color.a = 1.0f;

        break;
    }
    case BloomMode::UpsampleFirst:
    {
        float bloomTexWidth, bloomTexHeight, bloomMips;
        r_Texture.GetDimensions(int(c_Params.LodAndMode.x + 1.0f), bloomTexWidth, bloomTexHeight, bloomMips);
        float2 bloomTexSize = float2(bloomTexWidth, bloomTexHeight);

        float sampleScale = 1.0f;
        float3 upsampledTexture = UpsampleTent9(r_Texture, c_Params.LodAndMode.x + 1.0f, texCoords, 1.0f / bloomTexSize, sampleScale);

        float3 existing = r_Texture.SampleLevel(texCoords, c_Params.LodAndMode.x).rgb;
        color.rgb = existing + upsampledTexture;

        break;
    }
    case BloomMode::Upsample:
    {
        float bloomTexWidth, bloomTexHeight, bloomMips;
        r_BloomTexture.GetDimensions(int(c_Params.LodAndMode.x + 1.0f), bloomTexWidth, bloomTexHeight, bloomMips);
        float2 bloomTexSize = float2(bloomTexWidth, bloomTexHeight);

        float sampleScale = 1.0f;
        float3 upsampledTexture = UpsampleTent9(r_BloomTexture, c_Params.LodAndMode.x + 1.0f, texCoords, 1.0f / bloomTexSize, sampleScale);

        float3 existing = r_Texture.SampleLevel(texCoords, c_Params.LodAndMode.x).rgb;
        color.rgb = existing + upsampledTexture;

        break;
    }
    case BloomMode::Downsample:
    {
        color.rgb = DownsampleBox13(r_Texture, c_Params.LodAndMode.x, texCoords, 1.0f / texSize);

        break;
    }
    default:
        break;
    }

    o_Image[threadID.xy] = color;
}
