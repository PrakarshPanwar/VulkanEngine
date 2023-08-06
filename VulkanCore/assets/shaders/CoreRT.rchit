#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

struct RayPayload
{
	vec3 Color;
	vec4 ScatterDirection;
	float Distance;
	uint Seed;
};

layout(location = 0) rayPayloadInEXT RayPayload o_RayPayload;

hitAttributeEXT vec2 g_HitAttribs;

struct VertexSSBOLayout
{
	vec4 M0;
	vec4 M1;
	vec4 M2;
	vec4 M3;
	vec4 M4;
};

struct Vertex
{
	vec3 Position;
	vec3 Normal;
	vec3 Tangent;
	vec3 Binormal;
	vec3 Color;
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

struct MeshBufferData
{
	uint64_t VertexBufferAddress;
	uint64_t IndexBufferAddress;
};

layout(set = 0, binding = 3) uniform Camera
{
	mat4 Projection;
	mat4 View;
    mat4 InverseProjection;
	mat4 InverseView;
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

layout(set = 0, binding = 6) readonly buffer MeshData
{
	MeshBufferData addressData[];
} r_MeshBufferData;

layout(buffer_reference, scalar) buffer Vertices
{
	VertexSSBOLayout v[];
};

layout(buffer_reference, scalar) buffer Indices
{
	uint idx[];
};

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
    Material materials[];
} r_MaterialData;

// IBL
layout(set = 2, binding = 0) uniform samplerCube u_PrefilteredMap;
layout(set = 2, binding = 1) uniform samplerCube u_IrradianceMap;
layout(set = 2, binding = 2) uniform sampler2D u_BRDFTexture;

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
    vec3 tangentNormal = normalize(texture(u_NormalTextureArray[nonuniformEXT(gl_InstanceCustomIndexEXT)], Input.TexCoord).xyz * 2.0 - 1.0);
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

uint InitRandomSeed(uint val0, uint val1)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	[[unroll]] 
	for (uint n = 0; n < 16; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}

	return v0;
}

uint RandomInt(inout uint seed)
{
	// LCG values from Numerical Recipes
    return (seed = 1664525 * seed + 1013904223);
}

float RandomFloat(inout uint seed)
{
	// Float version using bitmask from Numerical Recipes
	const uint one = 0x3f800000;
	const uint msk = 0x007fffff;
	return uintBitsToFloat(one | (msk & (RandomInt(seed) >> 9))) - 1;
}

vec2 RandomInUnitDisk(inout uint seed)
{
	for (;;)
	{
		const vec2 p = 2 * vec2(RandomFloat(seed), RandomFloat(seed)) - 1;
		if (dot(p, p) < 1)
		{
			return p;
		}
	}
}

vec3 RandomInUnitSphere(inout uint seed)
{
	for (;;)
	{
		const vec3 p = 2 * vec3(RandomFloat(seed), RandomFloat(seed), RandomFloat(seed)) - 1;
		if (dot(p, p) < 1)
		{
			return p;
		}
	}
}

RayPayload ScatterMettalic(const vec3 color, const vec3 direction, const vec3 normal, const vec2 texCoord, const float hitDistance, inout uint seed)
{
    RayPayload rayPayload;
    
    const vec3 reflected = reflect(direction, normal);
	const vec4 scatter = vec4(reflected + m_Params.Roughness * RandomInUnitSphere(seed), dot(reflected, normal) > 0 ? 1 : 0);

    rayPayload.Color = color;
    rayPayload.ScatterDirection = scatter;
    rayPayload.Distance = hitDistance;
    rayPayload.Seed = seed;
	return rayPayload;
}

vec2 MixBarycentric(vec2 p0, vec2 p1, vec2 p2)
{
    const vec3 barycentricCoords = vec3(1.0 - g_HitAttribs.x - g_HitAttribs.y, g_HitAttribs.x, g_HitAttribs.y);
    vec2 res = p0 * barycentricCoords.x + p1 * barycentricCoords.y + p2 * barycentricCoords.z;

    return res;
}

vec3 MixBarycentric(vec3 p0, vec3 p1, vec3 p2)
{
    const vec3 barycentricCoords = vec3(1.0 - g_HitAttribs.x - g_HitAttribs.y, g_HitAttribs.x, g_HitAttribs.y);
    vec3 res = p0 * barycentricCoords.x + p1 * barycentricCoords.y + p2 * barycentricCoords.z;

    return res;
}

Vertex Unpack(int index)
{
	Vertices vertexData = Vertices(r_MeshBufferData.addressData[gl_InstanceCustomIndexEXT].VertexBufferAddress);	
	VertexSSBOLayout vertexLayout = vertexData.v[index];

	Vertex vertex;
	vertex.Position = vertexLayout.M0.xyz;
	vertex.Normal = vec3(vertexLayout.M0.w, vertexLayout.M1.xy);
	vertex.Tangent = vec3(vertexLayout.M1.zw, vertexLayout.M2.x);
	vertex.Binormal = vertexLayout.M2.yzw;
	vertex.Color = vertexLayout.M3.xyz;
	vertex.TexCoord = vec2(vertexLayout.M3.w, vertexLayout.M4.x);

	return vertex;
}

void main()
{
	Indices indexData = Indices(r_MeshBufferData.addressData[gl_InstanceCustomIndexEXT].IndexBufferAddress);
	ivec3 index = ivec3(indexData.idx[3 * gl_PrimitiveID], indexData.idx[3 * gl_PrimitiveID + 1], indexData.idx[3 * gl_PrimitiveID + 2]);
	
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
    Material materialData = r_MaterialData.materials[gl_InstanceCustomIndexEXT];

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
    vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;

    // Calculate Reflectance at Normal Incidence; if Di-Electric (like Plastic) use F0 
    // of 0.04 and if it's a Metal, use the Albedo color as F0 (Metallic Workflow)
    vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metallic);

    vec3 lightContribution = Lighting(F0);
    vec3 iblContribution = IBL(F0, Lr);

    //vec3 color = iblContribution + lightContribution;
    vec3 color = m_Params.Albedo.rgb + lightContribution;
    vec3 direction = normalize(gl_WorldRayDirectionEXT);
    o_RayPayload = ScatterMettalic(color, direction, m_Params.Normal, Input.TexCoord, gl_HitTEXT, o_RayPayload.Seed);
}
