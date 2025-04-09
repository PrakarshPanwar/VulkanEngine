// Shader File: "CorePBR.slang"

struct Camera
{
    float4x4 Projection;
    float4x4 View;
    float4x4 InverseView;
    float2 DepthUnpackConsts;
};

struct PointLight
{
    float4 Position;
    float4 Color;
    float Radius;
    float Falloff;
};

struct SpotLight
{
    float4 Position;
    float4 Color;
    float3 Direction;
    float InnerCutoff; // Cutoff Threshold
    float OuterCutoff; // Final Cutoff
    float Radius;
    float Falloff;
};

struct DirectionalLight
{
    float4 Color;
    float3 Direction;
    float Falloff;
};

struct PointLightData
{
    int Count;
    PointLight PointLights[10];
};

struct SpotLightData
{
    int Count;
    SpotLight SpotLights[10];
};

struct DirectionalLightData
{
    int Count;
    DirectionalLight DirectionLights[2];
};

struct Material
{
    float4 Albedo;
    float Emission;
    float Roughness;
    float Metallic;
    uint UseNormalMap;
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

[shader("vertex")]
VertexStageOutput VSMain(VertexInput vertexInput)
{
    VertexStageOutput output;

    float4x4 transform = float4x4(
		float4(vertexInput.MRow0.x, vertexInput.MRow1.x, vertexInput.MRow2.x, 0.0f),
		float4(vertexInput.MRow0.y, vertexInput.MRow1.y, vertexInput.MRow2.y, 0.0f),
        float4(vertexInput.MRow0.z, vertexInput.MRow1.z, vertexInput.MRow2.z, 0.0f),
		float4(vertexInput.MRow0.w, vertexInput.MRow1.w, vertexInput.MRow2.w, 1.0f)
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
[[vk::binding(0, 1)]] Sampler2D c_AlbedoTexture;
[[vk::binding(1, 1)]] Sampler2D c_NormalTexture;
[[vk::binding(2, 1)]] Sampler2D c_ARMTexture;

// IBL
[[vk::binding(6, 0)]] SamplerCube c_IrradianceMap;
[[vk::binding(7, 0)]] Sampler2D c_BRDFLUT;
[[vk::binding(8, 0)]] SamplerCube c_PrefilteredMap;
//[[vk::binding(9, 0)]] Sampler2DArray c_ShadowMap;

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
static const float PI = 3.14159265358979323846;

float3 GetNormalsFromMap()
{
    float3 tangentNormal = normalize(c_NormalTexture.Sample(Input.TexCoord).xyz * 2.0f - 1.0f);
    return normalize(mul(tangentNormal, Input.WorldNormals));
}

float3 Mix(float3 a, float3 b, float t)
{
    return a * (1.0f - t) + b * t;
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float dotNH = max(dot(N, H), 0.0f);
    float dotNH2 = dotNH * dotNH;

    float nom = a2;
    float denom = (dotNH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / max(denom, 1e-5);
}

float GeometrySchlickGGX(float dotNV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = dotNV;
    float denom = dotNV * (1.0f - k) + k;

    return nom / max(denom, 1e-5);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float dotNV = max(dot(N, V), 0.0f);
    float dotNL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(dotNV, roughness);
    float ggx1 = GeometrySchlickGGX(dotNL, roughness);

    return ggx1 * ggx2;
}

float GeometrySchlickSmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    float GL = dotNL / (dotNL * (1.0f - k) + k);
    float GV = dotNV / (dotNV * (1.0f - k) + k);
    return GL * GV;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0f - roughness), F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
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
        float intensity = clamp((theta - spotLight.InnerCutoff) / epsilon, 0.0f, 1.0f);

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

    float3 irradiance = c_IrradianceMap.Sample(m_Params.Normal).rgb;
    float3 diffuse = irradiance * m_Params.Albedo;

    // Sample both the Pre-Filter map and the BRDF LUT and combine them together as per the Split-Sum approximation to get the IBL Specular part
    const float MAX_REFLECTION_LOD = 4.0f;
    float3 prefilteredColor = c_PrefilteredMap.SampleLevel(Lr, m_Params.Roughness * MAX_REFLECTION_LOD).rgb;

    float2 brdf = c_BRDFLUT.Sample(float2(m_Params.NdotV, m_Params.Roughness)).rg;
    float3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    float3 ambient = (kD * diffuse + specular) * m_Params.Occlusion;

    return ambient;
}

[shader("fragment")]
void FSMain(VertexOutput vertexOutput : VertexOutput, out float4 oColor : SV_Target)
{
    Input = vertexOutput;

    float4 diffuse = c_AlbedoTexture.Sample(vertexOutput.TexCoord);
    m_Params.Albedo = diffuse.rgb * c_MaterialData.Albedo.rgb * c_MaterialData.Emission;

    // R->Ambient Occlusion, G->Roughness, B->Metallic
    float3 aorm = c_ARMTexture.Sample(vertexOutput.TexCoord).rgb;
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
    float3 F0 = Mix(Fdielectric, m_Params.Albedo, m_Params.Metallic);

    float3 lightingContribution = Lighting(F0);
    float3 iblContribution = IBL(F0, Lr);

    float3 color = iblContribution + lightingContribution;
    oColor = float4(color, 1.0f);
} // End of Fragment Stage
