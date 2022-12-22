#version 460 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec3 v_FragColor;
layout(location = 1) in vec3 v_FragPosWorld;
layout(location = 2) in vec3 v_FragNormalWorld;
layout(location = 3) in vec2 v_FragTexCoord;
layout(location = 4) in flat int v_TexIndex;

struct PointLight
{
	vec4 Position;
	vec4 Color;
};

// Buffer Data
layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InvView;
} u_Camera;

layout(set = 0, binding = 1) uniform PointLightData
{
	PointLight PointLights[10];
	int Count;
} u_PointLight;

// Material Data
layout(set = 0, binding = 2) uniform sampler2D u_DiffuseTextures[3];
layout(set = 0, binding = 3) uniform sampler2D u_NormalTextures[3];
layout(set = 0, binding = 4) uniform sampler2D u_AORoughMetalTextures[3];

// IBL
layout(binding = 5) uniform samplerCube u_IrradianceTexture;

const float PI = 3.14159265359;

vec3 GetNormalsFromMap()
{
    vec3 tangentNormal = texture(u_NormalTextures[v_TexIndex], v_FragTexCoord).xyz * 2.0 - 1.0;

    vec3 Q1 = dFdx(v_FragPosWorld);
    vec3 Q2 = dFdy(v_FragPosWorld);
    vec2 st1 = dFdx(v_FragTexCoord);
    vec2 st2 = dFdy(v_FragTexCoord);

    vec3 N = normalize(v_FragNormalWorld);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
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

    return nom / denom;
}

float GeometrySchlickGGX(float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = dotNV;
    float denom = dotNV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float dotNV = max(dot(N, V), 0.0);
    float dotNL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(dotNV, roughness);
    float ggx1 = GeometrySchlickGGX(dotNL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
	vec3 albedo = texture(u_DiffuseTextures[v_TexIndex], v_FragTexCoord).rgb;
    // R->Ambient Occlusion, G->Roughness, B->Metallic
    vec3 aorm = texture(u_AORoughMetalTextures[v_TexIndex], v_FragTexCoord).rgb;

    float ao = aorm.r;
    float roughness = aorm.g;
    float metallic = aorm.b;

	vec3 cameraPosWorld = u_Camera.InvView[3].xyz;
	vec3 V = normalize(cameraPosWorld - v_FragPosWorld);
    vec3 N = GetNormalsFromMap();
    vec3 R = reflect(-V, N);

    // Calculate Reflectance at Normal Incidence; if Di-Electric (like Plastic) use F0 
    // of 0.04 and if it's a Metal, use the Albedo color as F0 (Metallic Workflow)    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Reflectance Equation
    vec3 Lo = vec3(0.0);
	for (int i = 0; i < u_PointLight.Count; ++i)
	{
		PointLight pointLight = u_PointLight.PointLights[i];
		// Calculate Per-Light Radiance
        vec3 L = normalize(pointLight.Position.xyz - v_FragPosWorld);
        vec3 H = normalize(V + L);
        float dist = length(pointLight.Position.xyz - v_FragPosWorld);
        float attenuation = 1.0 / (dist * dist);
        vec3 radiance = pointLight.Color.xyz * pointLight.Color.w * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
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
        kD *= 1.0 - metallic;
            
        // Scale Light by dotNL
        float dotNL = max(dot(N, L), 0.0);

        // Add to Outgoing Radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * dotNL;
	}

    // Ambient Lighting (We now use IBL as the Ambient Term)
    vec3 kS = FresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(u_IrradianceTexture, N).rgb;
    vec3 diffuse = irradiance * albedo;
    vec3 ambient = (kD * diffuse) * ao;
    
    vec3 color = ambient + Lo;

	o_Color = vec4(color, 1.0);
}