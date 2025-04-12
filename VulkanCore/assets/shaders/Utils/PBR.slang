// Module File: "PBR.slang"

module PBR;
import Math;

public float DistributionGGX(float3 N, float3 H, float roughness)
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

public float GeometrySchlickGGX(float dotNV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = dotNV;
    float denom = dotNV * (1.0f - k) + k;

    return nom / max(denom, 1e-5);
}

public float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float dotNV = max(dot(N, V), 0.0f);
    float dotNL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(dotNV, roughness);
    float ggx1 = GeometrySchlickGGX(dotNL, roughness);

    return ggx1 * ggx2;
}

public float GeometrySchlickSmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    float GL = dotNL / (dotNL * (1.0f - k) + k);
    float GV = dotNV / (dotNV * (1.0f - k) + k);
    return GL * GV;
}

public float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

public float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0f - roughness), F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}
