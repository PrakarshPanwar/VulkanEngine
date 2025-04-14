// Shader File: "CorePBR.slang"

import Buffers;

struct Material
{
    float4 Albedo;
    float Emission;
    float Roughness;
    float Metallic;
    uint UseNormalMap;
};

struct CascadeData
{
    float4 CascadeSplitLevels;
    float4x4 LightSpaceMatrices[4];
};

struct VertexInput
{
    // Vertex Bindings
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float3 Normal;
    [[vk::location(2)]] float3 Tangent;
    [[vk::location(3)]] float3 Binormal;
    [[vk::location(4)]] float2 TexCoord;

    // Instance Bindings
    [[vk::location(5)]] float4 MRow0;
    [[vk::location(6)]] float4 MRow1;
    [[vk::location(7)]] float4 MRow2;
};

struct VertexOutput
{
    float3 WorldPosition;
    float3 ViewPosition;
    float3 Normal;
    float3x3 WorldNormals;
    float2 TexCoord;
};

struct VertexStageOutput
{
    VertexOutput vertexOutput : VertexOutput;
    float4 Position : SV_Position;
};

[[vk::push_constant]] ConstantBuffer<Material> c_MaterialData;

// Camera and Lights Set
[[vk::binding(0, 0)]] ConstantBuffer<Camera> c_Camera;
[[vk::binding(1, 0)]] ConstantBuffer<PointLightData> c_PointLightData;
[[vk::binding(2, 0)]] ConstantBuffer<SpotLightData> c_SpotLightData;
[[vk::binding(3, 0)]] ConstantBuffer<DirectionalLightData> c_DirectionalLightData;
[[vk::binding(4, 0)]] ConstantBuffer<CascadeData> c_CascadeData;

[shader("vertex")]
VertexStageOutput VSMain(VertexInput vertexInput)
{
    VertexStageOutput output;

    float4x4 transform = float4x4(
		vertexInput.MRow0.x, vertexInput.MRow1.x, vertexInput.MRow2.x, 0.0f,
		vertexInput.MRow0.y, vertexInput.MRow1.y, vertexInput.MRow2.y, 0.0f,
        vertexInput.MRow0.z, vertexInput.MRow1.z, vertexInput.MRow2.z, 0.0f,
		vertexInput.MRow0.w, vertexInput.MRow1.w, vertexInput.MRow2.w, 1.0f
	);

    float4 worldPosition = mul(float4(vertexInput.Position, 1.0f), transform);
    output.Position = mul(mul(worldPosition, c_Camera.View), c_Camera.Projection);
    output.vertexOutput.Normal = mul(vertexInput.Normal, float3x3(transform));
    output.vertexOutput.WorldPosition = worldPosition.xyz;
    output.vertexOutput.ViewPosition = mul(worldPosition, c_Camera.InverseView).xyz;

    float3 T = normalize(mul(vertexInput.Tangent, float3x3(transform)));
    float3 N = normalize(output.vertexOutput.Normal);
    T = normalize(T - dot(T, N) * N);

    float3 B = normalize(mul(vertexInput.Binormal, float3x3(transform)));

    output.vertexOutput.WorldNormals = float3x3(T, B, N);
    output.vertexOutput.TexCoord = float2(vertexInput.TexCoord.x, 1.0f - vertexInput.TexCoord.y);

    return output;
} // End of Vertex Stage

// Material Set
[[vk::binding(0, 1)]] Sampler2D r_AlbedoTexture;
[[vk::binding(1, 1)]] Sampler2D r_NormalTexture;
[[vk::binding(2, 1)]] Sampler2D r_ARMTexture;

// IBL
[[vk::binding(6, 0)]] SamplerCube r_IrradianceMap;
[[vk::binding(7, 0)]] Sampler2D r_BRDFLUT;
[[vk::binding(8, 0)]] SamplerCube r_PrefilteredMap;
[[vk::binding(9, 0)]] Sampler2DArray r_ShadowMap;

import PBR;
import Math;

struct PBRParams
{
    float3 View;
    float3 Normal;
    float NdotV;

    float3 Albedo;
    float Occlusion;
    float Metallic;
    float Roughness;
};

static PBRParams m_Params; // Thread-Local variable
static VertexOutput Input;

static const float3 Fdielectric = float3(0.04, 0.04, 0.04);

float3 GetNormalsFromMap()
{
    float3 tangentNormal = normalize(r_NormalTexture.Sample(Input.TexCoord).xyz * 2.0f - 1.0f);
    return normalize(mul(tangentNormal, Input.WorldNormals));
}

float3 Lighting(float3 F0)
{
    // Reflectance Equation
    float3 Lo = float3(0.0f);
    for (int i = 0; i < c_PointLightData.Count; ++i)
    {
        PointLight pointLight = c_PointLightData.PointLights[i];
        // Calculate Per-Light Radiance
        float3 L = normalize(pointLight.Position.xyz - Input.WorldPosition);
        float3 H = normalize(m_Params.View + L);
        float dist = length(pointLight.Position.xyz - Input.WorldPosition);

        // Calculating Attentuation
        float attenuation = 1.0f / (dist * (pointLight.Falloff + dist));

        float3 radiance = pointLight.Color.xyz * pointLight.Color.w * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(m_Params.Normal, H, m_Params.Roughness);
        float G = GeometrySmith(m_Params.Normal, m_Params.View, L, m_Params.Roughness);
        float3 F = FresnelSchlick(max(dot(H, m_Params.View), 0.0f), F0);

        float NdotL = max(dot(m_Params.Normal, L), 0.0);
        float3 numerator = NDF * G * F;
        float denominator = 4.0f * m_Params.NdotV * NdotL + 0.0001f; // + 0.0001 to prevent divide by zero
        float3 specular = numerator / denominator;

        // kS is equal to Fresnel
        float3 kS = F;
        // For Energy Conservation, the Diffuse and Specular Light can't
        // be above 1.0 (unless the Surface Emits Light); to preserve this
        // relationship the Diffuse Component (kD) should equal 1.0 - kS.
        float3 kD = float3(1.0f) - kS;
        // Multiply kD by the Inverse Metalness such that only non-metals
        // have Diffuse Lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0f - m_Params.Metallic;
        // Add to Outgoing Radiance Lo
        Lo += (kD * m_Params.Albedo / PI + specular) * radiance * NdotL;
    }

    for (int i = 0; i < c_SpotLightData.Count; ++i)
    {
        SpotLight spotLight = c_SpotLightData.SpotLights[i];
        // Calculate Per-Light Radiance
        float3 L = normalize(spotLight.Position.xyz - Input.WorldPosition);
        float3 H = normalize(m_Params.View + L);
        float dist = length(spotLight.Position.xyz - Input.WorldPosition);

        float theta = dot(L, normalize(-spotLight.Direction));
        float epsilon = spotLight.OuterCutoff - spotLight.InnerCutoff;
        float intensity = saturate((theta - spotLight.InnerCutoff) / epsilon);

        // Calculating Attentuation
        float attenuation = 1.0f / (dist * (spotLight.Falloff + dist));

        float3 radiance = spotLight.Color.xyz * spotLight.Color.w * intensity * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(m_Params.Normal, H, m_Params.Roughness);
        float G = GeometrySmith(m_Params.Normal, m_Params.View, L, m_Params.Roughness);
        float3 F = FresnelSchlick(max(dot(H, m_Params.View), 0.0f), F0);

        float NdotL = max(dot(m_Params.Normal, L), 0.0f);
        float3 numerator = NDF * G * F;
        float denominator = 4.0f * m_Params.NdotV * NdotL + 0.0001f; // + 0.0001 to prevent divide by zero
        float3 specular = numerator / denominator;

        // kS is equal to Fresnel
        float3 kS = F;
        // For Energy Conservation, the Diffuse and Specular Light can't
        // be above 1.0 (unless the Surface Emits Light); to preserve this
        // relationship the Diffuse Component (kD) should equal 1.0 - kS.
        float3 kD = float3(1.0f) - kS;
        // Multiply kD by the Inverse Metalness such that only non-metals
        // have Diffuse Lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0f - m_Params.Metallic;
        // Add to Outgoing Radiance Lo
        Lo += (kD * m_Params.Albedo / PI + specular) * radiance * NdotL;
    }

    for (int i = 0; i < c_DirectionalLightData.Count; ++i)
    {
        DirectionalLight directionLight = c_DirectionalLightData.DirectionLights[i];
        // Calculate Per-Light Radiance
        float3 L = normalize(-directionLight.Direction);
        float3 H = normalize(m_Params.View + L);

        // Calculating Attentuation
        float attenuation = 1.0f / directionLight.Falloff;

        float3 radiance = directionLight.Color.xyz * directionLight.Color.w * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(m_Params.Normal, H, m_Params.Roughness);
        float G = GeometrySmith(m_Params.Normal, m_Params.View, L, m_Params.Roughness);
        float3 F = FresnelSchlick(max(dot(H, m_Params.View), 0.0f), F0);

        float NdotL = max(dot(m_Params.Normal, L), 0.0f);
        float3 numerator = NDF * G * F;
        float denominator = 4.0f * m_Params.NdotV * NdotL + 0.0001f; // + 0.0001 to prevent divide by zero
        float3 specular = numerator / denominator;

        // kS is equal to Fresnel
        float3 kS = F;
        // For Energy Conservation, the Diffuse and Specular Light can't
        // be above 1.0 (unless the Surface Emits Light); to preserve this
        // relationship the Diffuse Component (kD) should equal 1.0 - kS.
        float3 kD = float3(1.0f) - kS;
        // Multiply kD by the Inverse Metalness such that only non-metals
        // have Diffuse Lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0f - m_Params.Metallic;
        // Add to Outgoing Radiance Lo
        Lo += (kD * m_Params.Albedo / PI + specular) * radiance * NdotL;
    }

    return Lo;
}

float3 IBL(float3 F0, float3 Lr)
{
    // Ambient Lighting (We now use IBL as the Ambient Term)
    float3 F = FresnelSchlickRoughness(m_Params.NdotV, F0, m_Params.Roughness);

    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - m_Params.Metallic;

    float3 irradiance = r_IrradianceMap.Sample(m_Params.Normal).rgb;
    float3 diffuse = irradiance * m_Params.Albedo;

    // Sample both the Pre-Filter map and the BRDF LUT and combine them together as per the Split-Sum approximation to get the IBL Specular part
    const float MAX_REFLECTION_LOD = 4.0f;
    float3 prefilteredColor = r_PrefilteredMap.SampleLevel(Lr, m_Params.Roughness * MAX_REFLECTION_LOD).rgb;

    float2 brdf = r_BRDFLUT.Sample(float2(m_Params.NdotV, m_Params.Roughness)).rg;
    float3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    float3 ambient = (kD * diffuse + specular) * m_Params.Occlusion;

    return ambient;
}

static const float4x4 BIAS_MATRIX = float4x4(
	0.5f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.5f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.0f, 1.0f
);

float LinearizeDepth(const float screenDepth)
{
    float depthLinearizeMul = c_Camera.DepthUnpackConsts.x;
    float depthLinearizeAdd = c_Camera.DepthUnpackConsts.y;
    return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

float ShadowTextureProjection(float4 coords, float2 offset, uint index)
{
    float shadow = 1.0f;
    float bias = 0.005f;

    if (coords.z > -1.0f && coords.z < 1.0f)
    {
        float dist = r_ShadowMap.Sample(float3(coords.xy + offset, index)).r;
        if (coords.w > 0 && dist < coords.z - bias)
            shadow = 0.1f; // Ambient Value
    }

    return shadow;
}

float FilterPCF(float4 coords, uint index)
{
    float texWidth, texHeight, texElements;
    r_ShadowMap.GetDimensions(texWidth, texHeight, texElements);

    float scale = 0.75f;
    float dx = scale / texWidth;
    float dy = scale / texHeight;

    int count = 0;
    int range = 1;

    float shadowFactor = 0.0f;
    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            shadowFactor += ShadowTextureProjection(coords, float2(dx * x, dy * y), index);
            count++;
        }
    }

    return shadowFactor / count;
}

float CalculateShadow()
{
    uint cascadeIndex = 0;
    for (uint i = 0; i < 3; ++i)
    {
        if (Input.ViewPosition.z < c_CascadeData.CascadeSplitLevels[i])
            cascadeIndex = i + 1;
    }

    // Depth Compare for Shadowing
    float4 shadowCoord = mul(float4(Input.WorldPosition, 1.0f), mul(c_CascadeData.LightSpaceMatrices[cascadeIndex], BIAS_MATRIX));
    shadowCoord = shadowCoord / shadowCoord.w;

    float shadow = FilterPCF(shadowCoord, cascadeIndex);
    //float shadow = ShadowTextureProjection(shadowCoord / shadowCoord.w, float2(0.0), cascadeIndex);

    return shadow;
}

[shader("fragment")]
void FSMain(VertexOutput vertexOutput : VertexOutput, out float4 o_Color : SV_Target)
{
    Input = vertexOutput;

    float4 diffuse = r_AlbedoTexture.Sample(vertexOutput.TexCoord);
    m_Params.Albedo = diffuse.rgb * c_MaterialData.Albedo.rgb * c_MaterialData.Emission;

    // R->Ambient Occlusion, G->Roughness, B->Metallic
    float3 aorm = r_ARMTexture.Sample(vertexOutput.TexCoord).rgb;
    m_Params.Occlusion = aorm.r;
    m_Params.Roughness = aorm.g * c_MaterialData.Roughness;
    m_Params.Metallic = aorm.b * c_MaterialData.Metallic;

    float3 cameraPosition = c_Camera.InverseView[3].xyz;
    m_Params.View = normalize(cameraPosition - vertexOutput.WorldPosition);
    m_Params.Normal = c_MaterialData.UseNormalMap == 0 ? normalize(vertexOutput.Normal) : GetNormalsFromMap();

    m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0f);
    float3 Lr = 2.0f * m_Params.NdotV * m_Params.Normal - m_Params.View;

    // Calculate Reflectance at Normal Incidence; if Di-Electric (like Plastic) use F0
    // of 0.04 and if it's a Metal, use the Albedo color as F0 (Metallic Workflow)
    float3 F0 = lerp(Fdielectric, m_Params.Albedo, m_Params.Metallic);

    // Shadow
    float shadow = CalculateShadow();

    float3 lightingContribution = Lighting(F0) * shadow;
    float3 iblContribution = IBL(F0, Lr);

    float3 color = iblContribution + lightingContribution;
    o_Color = float4(color, c_MaterialData.Albedo.a);
} // End of Fragment Stage
