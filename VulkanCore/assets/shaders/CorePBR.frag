#version 460 core

layout(location = 0) out vec4 o_Color;

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
    mat3 WorldNormals;
	vec2 TexCoord;
	vec3 VertexColor;
};

layout(location = 0) in VertexOutput Input;

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

layout(push_constant) uniform Material
{
    vec4 Albedo;
    float Roughness;
    float Metallic;
    uint UseNormalMap;
} u_Material;

// Buffer Data
layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
    vec2 DepthUnpackConsts;
    vec2 CameraTanHalfFOV;
} u_Camera;

layout(set = 0, binding = 1) uniform PointLightData
{
	int Count;
	PointLight PointLights[10];
} u_PointLight;

layout(set = 0, binding = 2) uniform SpotLightData
{
    int Count;
    SpotLight SpotLights[10];
} u_SpotLight;

// Material Set
layout(set = 1, binding = 0) uniform sampler2D u_DiffuseTexture;
layout(set = 1, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 1, binding = 2) uniform sampler2D u_ARMTexture;

// IBL
layout(set = 0, binding = 6) uniform samplerCube u_IrradianceMap;
layout(set = 0, binding = 7) uniform sampler2D u_BRDFTexture;
layout(set = 0, binding = 8) uniform samplerCube u_PrefilteredMap;

const float PI = 3.14159265359;

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
    vec3 tangentNormal = normalize(texture(u_NormalTexture, Input.TexCoord).xyz * 2.0 - 1.0);
    return normalize(Input.WorldNormals * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float dotNH = max(dot(N, H), 0.0);
    float dotNH2 = dotNH * dotNH;

    float nom   = a2;
    float denom = (dotNH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 1e-5);
}

float GeometrySchlickGGX(float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = dotNV;
    float denom = dotNV * (1.0 - k) + k;

    return nom / max(denom, 1e-5);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float dotNV = max(dot(N, V), 0.0);
    float dotNL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(dotNV, roughness);
    float ggx1 = GeometrySchlickGGX(dotNL, roughness);

    return ggx1 * ggx2;
}

float GeometrySchlickSmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(u_PrefilteredMap, Lr, m_Params.Roughness * MAX_REFLECTION_LOD).rgb;

    vec2 brdf = texture(u_BRDFTexture, vec2(m_Params.NdotV, m_Params.Roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * m_Params.Occlusion;

    return ambient;
}

void main()
{
	m_Params.Albedo = texture(u_DiffuseTexture, Input.TexCoord).rgb * u_Material.Albedo.rgb * u_Material.Albedo.a;
    // R->Ambient Occlusion, G->Roughness, B->Metallic
    vec3 aorm = texture(u_ARMTexture, Input.TexCoord).rgb;

    m_Params.Occlusion = aorm.r;
    m_Params.Roughness = aorm.g * u_Material.Roughness;
    m_Params.Metallic = aorm.b * u_Material.Metallic;

	vec3 cameraPosWorld = u_Camera.InverseView[3].xyz;
	m_Params.View = normalize(cameraPosWorld - Input.WorldPosition);
    m_Params.Normal = u_Material.UseNormalMap == 0 ? normalize(Input.Normal) : GetNormalsFromMap();
    m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
    vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;

    // Calculate Reflectance at Normal Incidence; if Di-Electric (like Plastic) use F0 
    // of 0.04 and if it's a Metal, use the Albedo color as F0 (Metallic Workflow)
    vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metallic);

    vec3 lightContribution = Lighting(F0);
    vec3 iblContribution = IBL(F0, Lr);

    vec3 color = iblContribution + lightContribution;

	o_Color = vec4(color, 1.0);
}
