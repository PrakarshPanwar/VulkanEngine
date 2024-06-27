#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

hitAttributeEXT vec2 g_HitAttribs;

#include "Utils/Payload.glslh"
#include "Utils/Random.glslh"
#include "Utils/Barycentric.glslh"
#include "Utils/Math.glslh"
#include "Utils/Sampling.glslh"

layout(location = 0) rayPayloadInEXT RayPayload o_RayPayload;
layout(location = 1) rayPayloadInEXT bool o_CastShadow;

struct VertexLayout
{
	vec4 M0;
	vec4 M1;
	vec4 M2;
	vec4 M3;
};

struct Vertex
{
	vec3 Position;
	vec3 Normal;
	vec3 Tangent;
	vec3 Binormal;
	vec2 TexCoord;
};

struct VertexInput
{
	vec3 WorldPosition;
	vec3 Normal;
    mat3 WorldNormals;
	vec2 TexCoord;
} Input;

struct PointLight
{
    vec4 Position;
	vec4 Color;
    float Radius;
    float Falloff;
};

struct SpotLight
{
    vec4 Position;
    vec4 Color;
    vec3 Direction;
    float InnerCutoff;
    float OuterCutoff;
    float Radius;
    float Falloff;
};

layout(set = 0, binding = 0) uniform accelerationStructureEXT u_TopLevelAS;

layout(set = 0, binding = 3) uniform Camera
{
	mat4 Projection;
	mat4 View;
    mat4 InverseProjection;
	mat4 InverseView;
    vec2 DepthUnpackConsts;
} u_Camera;

layout(set = 0, binding = 4) uniform PointLightData
{
	int Count;
	PointLight PointLights[10];
} u_PointLight;

layout(set = 0, binding = 5) uniform SpotLightData
{
    int Count;
    SpotLight SpotLights[10];
} u_SpotLight;

layout(buffer_reference, scalar) buffer Vertices
{
	VertexLayout Vert[];
};

layout(buffer_reference, scalar) buffer Indices
{
	uint Idx[];
};

struct MeshInfo
{
	Vertices VBAddress;
	Indices IBAddress;
};

layout(set = 0, binding = 6) readonly buffer MeshInfoData
{
	MeshInfo Data[];
} r_MeshInfo;

// Materials Texture Array
layout(set = 1, binding = 0) uniform sampler2D u_DiffuseTextureArray[];
layout(set = 1, binding = 1) uniform sampler2D u_NormalTextureArray[];
layout(set = 1, binding = 2) uniform sampler2D u_ARMTextureArray[];

struct Material
{
    vec4 Albedo;
    float Emission;
    float Roughness;
    float Metallic;
    uint UseNormalMap;
};

layout(set = 1, binding = 3) readonly buffer MaterialData
{
    Material MatBuffer[];
} r_MaterialData;

layout(set = 1, binding = 4) uniform RTMaterialData
{
    float Transmission;
	float SpecularTint;
	float IOR;
	float Sheen;
	float SheenTint;
	float Clearcoat;
	float ClearcoatGloss;
	float Subsurface;
	float Extinction;
	float AtDistance;
} u_RTMaterialData;

#include "Utils/Lighting.glslh"

// IBL
layout(set = 2, binding = 0) uniform samplerCube u_PrefilteredMap;
layout(set = 2, binding = 1) uniform samplerCube u_IrradianceMap;
layout(set = 2, binding = 2) uniform sampler2D u_BRDFTexture;

// Fresnel factor for all incidence
const vec3 Fdielectric = vec3(0.04);

struct PBRParams
{
    vec3 View;
    vec3 Normal;
    float NdotV;

    vec3 Albedo;
    float Occlusion;
    float Metallic;
    float Roughness;
} m_Params;

vec3 GetNormalsFromMap()
{
    vec3 tangentNormal = normalize(texture(u_NormalTextureArray[nonuniformEXT(gl_InstanceCustomIndexEXT)], Input.TexCoord).xyz * 2.0 - 1.0);
    return normalize(Input.WorldNormals * tangentNormal);
}

vec3 Lighting(vec3 F0)
{
    // Reflectance Equation
    vec3 Lo = vec3(0.0);
	for (int i = 0; i < u_PointLight.Count; ++i)
	{
		PointLight pointLight = u_PointLight.PointLights[i];
		// Calculate Per-Light Radiance
        vec3 L = normalize(pointLight.Position.xyz - Input.WorldPosition);
        vec3 H = normalize(m_Params.View + L);
        float dist = length(pointLight.Position.xyz - Input.WorldPosition);

        // Calculating Attentuation
        float attenuation = (1.0) / (dist * (pointLight.Falloff + dist));

        vec3 radiance = pointLight.Color.xyz * pointLight.Color.w * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(m_Params.Normal, H, m_Params.Roughness);
        float G = GeometrySmith(m_Params.Normal, m_Params.View, L, m_Params.Roughness);
        vec3 F = FresnelSchlick(max(dot(H, m_Params.View), 0.0), F0);
        
        float NdotL = max(dot(m_Params.Normal, L), 0.0);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * m_Params.NdotV * NdotL + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // For Energy Conservation, the Diffuse and Specular Light can't
        // be above 1.0 (unless the Surface Emits Light); to preserve this
        // relationship the Diffuse Component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // Multiply kD by the Inverse Metalness such that only non-metals 
        // have Diffuse Lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - m_Params.Metallic;
        // Add to Outgoing Radiance Lo
        Lo += (kD * m_Params.Albedo / PI + specular) * radiance * NdotL;
	}

    for (int i = 0; i < u_SpotLight.Count; ++i)
    {
        SpotLight spotLight = u_SpotLight.SpotLights[i];
		// Calculate Per-Light Radiance
        vec3 L = normalize(spotLight.Position.xyz - Input.WorldPosition);
        vec3 H = normalize(m_Params.View + L);
        float dist = length(spotLight.Position.xyz - Input.WorldPosition);

        float theta = dot(L, normalize(-spotLight.Direction)); 
        float epsilon = (spotLight.InnerCutoff - spotLight.OuterCutoff);
        float intensity = clamp((theta - spotLight.OuterCutoff) / epsilon, 0.0, 1.0);

        // Calculating Attentuation
        float attenuation = (1.0) / (dist * (spotLight.Falloff + dist));

        vec3 radiance = spotLight.Color.xyz * spotLight.Color.w * intensity * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(m_Params.Normal, H, m_Params.Roughness);
        float G = GeometrySmith(m_Params.Normal, m_Params.View, L, m_Params.Roughness);
        vec3 F = FresnelSchlick(max(dot(H, m_Params.View), 0.0), F0);
        
        float NdotL = max(dot(m_Params.Normal, L), 0.0);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * m_Params.NdotV * NdotL + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // For Energy Conservation, the Diffuse and Specular Light can't
        // be above 1.0 (unless the Surface Emits Light); to preserve this
        // relationship the Diffuse Component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // Multiply kD by the Inverse Metalness such that only non-metals 
        // have Diffuse Lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - m_Params.Metallic;
        // Add to Outgoing Radiance Lo
        Lo += (kD * m_Params.Albedo / PI + specular) * radiance * NdotL;
    }

    return Lo;
}

vec3 IBL(vec3 F0, vec3 Lr)
{
    // Ambient Lighting (We now use IBL as the Ambient Term)
    vec3 F = FresnelSchlickRoughness(m_Params.NdotV, F0, m_Params.Roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - m_Params.Metallic;

    vec3 irradiance = texture(u_IrradianceMap, m_Params.Normal).rgb;
    vec3 diffuse = irradiance * m_Params.Albedo;

    // Sample both the Pre-Filter map and the BRDF LUT and combine them together as per the Split-Sum approximation to get the IBL Specular part
    int MaxTextureLod = textureQueryLevels(u_PrefilteredMap);
    vec3 prefilteredColor = textureLod(u_PrefilteredMap, Lr, m_Params.Roughness * MaxTextureLod).rgb;

    vec2 brdf = texture(u_BRDFTexture, vec2(m_Params.NdotV, m_Params.Roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * m_Params.Occlusion;

    return ambient;
}

vec3 EvalDielectricReflection(vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (dot(N, L) <= 0.0)
        return vec3(0.0);

    float eta = o_RayPayload.ETA;
    float F = FresnelDielectric(dot(V, H), eta);
    float D = GTR2(dot(N, H), m_Params.Roughness);

    pdf = D * dot(N, H) * F / (4.0 * abs(dot(V, H)));
    float G = GeometrySmithGGX(abs(dot(N, L)), m_Params.Roughness) * GeometrySmithGGX(abs(dot(N, V)), m_Params.Roughness);
    
    return m_Params.Albedo * F * D * G;
}

vec3 EvalDielectricRefraction(vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    float eta = o_RayPayload.ETA;

    pdf = 0.0;
    if (dot(N, L) >= 0.0)
        return vec3(0.0);

    float F = FresnelDielectric(abs(dot(V, H)), eta);
    float D = GTR2(dot(N, H), m_Params.Roughness);
    float denomSqrt = dot(L, H) + dot(V, H) * eta;
    pdf = D * dot(N, H) * (1.0 - F) * abs(dot(L, H)) / (denomSqrt * denomSqrt);
    float G = GeometrySmithGGX(abs(dot(N, L)), m_Params.Roughness) * GeometrySmithGGX(abs(dot(N, V)), m_Params.Roughness);

    return m_Params.Albedo * (1.0 - F) * D * G * abs(dot(V, H)) * abs(dot(L, H)) * 4.0 * eta * eta / (denomSqrt * denomSqrt);
}

vec3 EvalSpecular(in vec3 Cspec0, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (dot(N, L) <= 0.0)
        return vec3(0.0);

    float D = GTR2(dot(N, H), m_Params.Roughness);
    pdf = D * dot(N, H) / (4.0 * dot(V, H));
    float FH = FresnelSchlick(dot(L, H));
    vec3 F = mix(Cspec0, vec3(1.0), FH);
    float G = GeometrySmithGGX(abs(dot(N, L)), m_Params.Roughness) * GeometrySmithGGX(abs(dot(N, V)), m_Params.Roughness);

    return F * D * G;
}

vec3 EvalClearcoat(vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (dot(N, L) <= 0.0)
        return vec3(0.0);

    float D = GTR1(dot(N, H), mix(0.1, 0.001, u_RTMaterialData.ClearcoatGloss));
    pdf = D * dot(N, H) / (4.0 * dot(V, H));
    float FH = FresnelSchlick(dot(L, H));
    float F = mix(0.04, 1.0, FH);
    float G = GeometrySmithGGX(dot(N, L), 0.25) * GeometrySmithGGX(dot(N, V), 0.25);

    return vec3(0.25 * u_RTMaterialData.Clearcoat * F * D * G);
}

vec3 EvalDiffuse(in vec3 Csheen, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (dot(N, L) <= 0.0)
        return vec3(0.0);

    pdf = dot(N, L) * (1.0 / PI);

    // Diffuse
    float FL = FresnelSchlick(dot(N, L));
    float FV = FresnelSchlick(dot(N, V));
    float FH = FresnelSchlick(dot(L, H));
    float Fd90 = 0.5 + 2.0 * dot(L, H) * dot(L, H) * m_Params.Roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // Fake Subsurface TODO: Replace with Volumetric Scattering
    float Fss90 = dot(L, H) * dot(L, H) * m_Params.Roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (dot(N, L) + dot(N, V)) - 0.5) + 0.5);

    vec3 Fsheen = FH * u_RTMaterialData.Sheen * Csheen;

    return ((1.0 / PI) * mix(Fd, ss, u_RTMaterialData.Subsurface) * m_Params.Albedo + Fsheen) * (1.0 - m_Params.Metallic);
}

vec3 DisneySample(in Material material, inout vec3 L, inout float pdf)
{
    pdf = 0.0;
    vec3 Fr = vec3(0.0);

    float diffuseRatio = 0.5 * (1.0 - m_Params.Metallic);
    float transWeight = (1.0 - m_Params.Metallic) * u_RTMaterialData.Transmission;

    vec3 Cdlin = m_Params.Albedo;
    float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // Luminance approximation

    vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0); // normalize Luminance to isolate Hue + Saturation
    vec3 Cspec0 = mix(material.Albedo.w * 0.08 * mix(vec3(1.0), Ctint, u_RTMaterialData.SpecularTint), Cdlin, m_Params.Metallic);
    vec3 Csheen = mix(vec3(1.0), Ctint, u_RTMaterialData.SheenTint);
    float eta = o_RayPayload.ETA;

    vec3 N = o_RayPayload.FFNormal;
    vec3 V = -gl_WorldRayDirectionEXT;

    float r1 = RandomFloat(o_RayPayload.Seed);
    float r2 = RandomFloat(o_RayPayload.Seed);

    if (RandomFloat(o_RayPayload.Seed) < transWeight)
    {
        vec3 H = ImportanceSampleGTR2(m_Params.Roughness, r1, r2);
        H = Input.WorldNormals * H;

        if (dot(V, H) < 0.0)
            H = -H;

        vec3 R = reflect(-V, H);
        float F = FresnelDielectric(abs(dot(R, H)), eta);

        // Reflection/Total Internal Reflection
        if (r2 < F)
        {
            L = normalize(R);
            Fr = EvalDielectricReflection(V, N, L, H, pdf);
        }
        else // Transmission
        {
            L = normalize(refract(-V, H, eta));
            Fr = EvalDielectricRefraction(V, N, L, H, pdf);
        }

        Fr *= transWeight;
        pdf *= transWeight;
    }
    else
    {
        if (RandomFloat(o_RayPayload.Seed) < diffuseRatio)
        {
            L = CosSampleHemisphere(o_RayPayload.Seed);
            L = Input.WorldNormals * L;

            vec3 H = normalize(L + V);

            Fr = EvalDiffuse(Csheen, V, N, L, H, pdf);
            pdf *= diffuseRatio;
        }
        else // Specular
        {
            float primarySpecRatio = 1.0 / (1.0 + u_RTMaterialData.Clearcoat);

            // Sample Primary Specular Lobe
            if (RandomFloat(o_RayPayload.Seed) < primarySpecRatio)
            {
                // TODO: Implement http://jcgt.org/published/0007/04/01/
                vec3 H = ImportanceSampleGTR2(m_Params.Roughness, r1, r2);
                H = Input.WorldNormals * H;

                if (dot(V, H) < 0.0)
                    H = -H;

                L = normalize(reflect(-V, H));

                Fr = EvalSpecular(Cspec0, V, N, L, H, pdf);
                pdf *= primarySpecRatio * (1.0 - diffuseRatio);
            }
            else // Sample Clearcoat Lobe
            {
                vec3 H = ImportanceSampleGTR1(mix(0.1, 0.001, u_RTMaterialData.ClearcoatGloss), r1, r2);
                H = Input.WorldNormals * H;

                if (dot(V, H) < 0.0)
                    H = -H;

                L = normalize(reflect(-V, H));

                Fr = EvalClearcoat(V, N, L, H, pdf);
                pdf *= (1.0 - primarySpecRatio) * (1.0 - diffuseRatio);
            }
        }

        Fr *= (1.0 - transWeight);
        pdf *= (1.0 - transWeight);
    }

    return Fr;
}

vec3 DisneyEval(Material material, vec3 L, inout float pdf)
{
    vec3 N = o_RayPayload.FFNormal;
    vec3 V = -gl_WorldRayDirectionEXT;
    float eta = o_RayPayload.ETA;

    vec3 H = vec3(0.0);
    bool reflection = dot(N, L) > 0.0;

    if (reflection)
        H = normalize(L + V);
    else
        H = normalize(L + V * eta);

    if (dot(V, H) < 0.0)
        H = -H;

    float diffuseRatio = 0.5 * (1.0 - m_Params.Metallic);
    float primarySpecRatio = 1.0 / (1.0 + u_RTMaterialData.Clearcoat);
    float transWeight = (1.0 - m_Params.Metallic) * u_RTMaterialData.Transmission;

    vec3 brdf = vec3(0.0);
    vec3 bsdf = vec3(0.0);
    float brdfPdf = 0.0;
    float bsdfPdf = 0.0;

    if (transWeight > 0.0)
    {
        // Reflection
        if (reflection)
        {
            bsdf = EvalDielectricReflection(V, N, L, H, bsdfPdf);
        }
        else // Transmission
        {
            bsdf = EvalDielectricRefraction(V, N, L, H, bsdfPdf);
        }
    }

    float tpdf = 0.0;
    if (transWeight < 1.0)
    {
        vec3 Cdlin = m_Params.Albedo.xyz;
        float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

        vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0f); // normalize lum. to isolate hue+sat
        vec3 Cspec0 = mix(material.Albedo.w * 0.08 * mix(vec3(1.0), Ctint, u_RTMaterialData.SpecularTint), Cdlin, m_Params.Metallic);
        vec3 Csheen = mix(vec3(1.0), Ctint, u_RTMaterialData.SheenTint);

        // Diffuse
        brdf += EvalDiffuse(Csheen, V, N, L, H, tpdf);
        brdfPdf += tpdf * diffuseRatio;

        // Specular
        brdf += EvalSpecular(Cspec0, V, N, L, H, tpdf);
        brdfPdf += tpdf * primarySpecRatio * (1.0 - diffuseRatio);

        // Clearcoat
        brdf += EvalClearcoat(V, N, L, H, tpdf);
        brdfPdf += tpdf * (1.0 - primarySpecRatio) * (1.0 - diffuseRatio);
    }

    pdf = mix(brdfPdf, bsdfPdf, transWeight);

    return mix(brdf, bsdf, transWeight);
}

#include "Utils/DirectLight.glslh"

Vertex Unpack(int index)
{
	Vertices vertexData = Vertices(r_MeshInfo.Data[gl_InstanceCustomIndexEXT].VBAddress);	
	VertexLayout vertexLayout = vertexData.Vert[index];

	Vertex vertex;
	vertex.Position = vertexLayout.M0.xyz;
	vertex.Normal = vec3(vertexLayout.M0.w, vertexLayout.M1.xy);
	vertex.Tangent = vec3(vertexLayout.M1.zw, vertexLayout.M2.x);
	vertex.Binormal = vertexLayout.M2.yzw;
	vertex.TexCoord = vec2(vertexLayout.M3.x, vertexLayout.M3.y);

	return vertex;
}

void main()
{
	Indices indexData = Indices(r_MeshInfo.Data[gl_InstanceCustomIndexEXT].IBAddress);
	ivec3 index = ivec3(
        indexData.Idx[3 * gl_PrimitiveID],
        indexData.Idx[3 * gl_PrimitiveID + 1],
        indexData.Idx[3 * gl_PrimitiveID + 2]);
	
	Vertex v0 = Unpack(index.x);
	Vertex v1 = Unpack(index.y);
	Vertex v2 = Unpack(index.z);

	// Interpolate Vertex Data
	vec3 normal = MixBarycentric(v0.Normal, v1.Normal, v2.Normal);
    vec3 tangent = MixBarycentric(v0.Tangent, v1.Tangent, v2.Tangent);
    vec3 binormal = MixBarycentric(v0.Binormal, v1.Binormal, v2.Binormal);
	vec2 texCoord = MixBarycentric(v0.TexCoord, v1.TexCoord, v2.TexCoord);

    // Set Input Data
    Input.WorldPosition = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    Input.Normal = normalize(vec3(normal * gl_WorldToObjectEXT));
    Input.TexCoord = vec2(texCoord.x, 1.0 - texCoord.y);

    vec3 T = normalize(vec3(tangent * gl_WorldToObjectEXT));
    vec3 N = Input.Normal;
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    Input.WorldNormals = mat3(T, B, N);

    // Set Materials
    Material materialData = r_MaterialData.MatBuffer[gl_InstanceCustomIndexEXT];

	vec4 diffuse = texture(u_DiffuseTextureArray[nonuniformEXT(gl_InstanceCustomIndexEXT)], Input.TexCoord);
	m_Params.Albedo = diffuse.rgb * materialData.Albedo.rgb * materialData.Emission;
    // R->Ambient Occlusion, G->Roughness, B->Metallic
    vec3 aorm = texture(u_ARMTextureArray[nonuniformEXT(gl_InstanceCustomIndexEXT)], Input.TexCoord).rgb;

    m_Params.Occlusion = aorm.r;
    m_Params.Roughness = aorm.g * materialData.Roughness;
    m_Params.Metallic = aorm.b * materialData.Metallic;

	vec3 cameraPosWorld = u_Camera.InverseView[3].xyz;
	m_Params.View = normalize(cameraPosWorld - Input.WorldPosition);
    m_Params.Normal = materialData.UseNormalMap == 0 ? normalize(Input.Normal) : GetNormalsFromMap();
    m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);

    // Face Forward Normal
    vec3 ffNormal = dot(m_Params.Normal, gl_WorldRayDirectionEXT) <= 0.0 ? m_Params.Normal : -m_Params.Normal;
    float eta = dot(m_Params.Normal, ffNormal) > 0.0 ? (1.0 / u_RTMaterialData.IOR) : u_RTMaterialData.IOR;

    o_RayPayload.WorldPosition = Input.WorldPosition;
    o_RayPayload.Normal = m_Params.Normal;
    o_RayPayload.FFNormal = ffNormal;
    o_RayPayload.ETA = eta;
    o_RayPayload.Distance = gl_HitTEXT;

    // Calculate Reflectance at Normal Incidence; if Di-Electric (like Plastic) use F0
    // of 0.04 and if it's a Metal, use the Albedo color as F0 (Metallic Workflow)
    //vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metallic);

    BSDFSample bsdfSample;
	LightSample lightSample;

	o_RayPayload.Radiance += m_Params.Albedo * o_RayPayload.Beta;

	if (IntersectsEmitter(lightSample, gl_HitTEXT))
	{
		vec3 Le = SampleEmitter(lightSample, o_RayPayload.BSDF);

		if (dot(o_RayPayload.FFNormal, lightSample.Normal) > 0.0)
			o_RayPayload.Radiance += Le * o_RayPayload.Beta;

		o_RayPayload.Distance = -1.0;
		return;
	}

	o_RayPayload.Beta *= exp(-o_RayPayload.Absorption * gl_HitTEXT);
	o_RayPayload.Radiance += DirectLight(materialData) * o_RayPayload.Beta;

	vec3 F = DisneySample(materialData, bsdfSample.Direction, bsdfSample.PDF);
	float cosTheta = abs(dot(ffNormal, bsdfSample.Direction));

	if (bsdfSample.PDF <= 0.0)
	{
		o_RayPayload.Distance = -1.0;
		return;
	}

	o_RayPayload.Beta *= F * cosTheta / (bsdfSample.PDF + EPS);

	if (dot(ffNormal, bsdfSample.Direction) < 0.0)
		o_RayPayload.Absorption = -log(vec3(u_RTMaterialData.Extinction)) / (u_RTMaterialData.AtDistance + EPS);

	// Russian Roulette
	if (Max3(o_RayPayload.Beta) < 0.01 && o_RayPayload.Depth > 2)
	{
		float q = max(float(0.05), 1.0 - Max3(o_RayPayload.Beta));
		if (RandomFloat(o_RayPayload.Seed) < q)
			o_RayPayload.Distance = -1.0;

		o_RayPayload.Beta /= (1.0 - q);
	}

	o_RayPayload.BSDF = bsdfSample;

	// Update a new Ray path Bounce Direction
	o_RayPayload.RayData.Direction = bsdfSample.Direction;
	o_RayPayload.RayData.Origin = Input.WorldPosition;
}
