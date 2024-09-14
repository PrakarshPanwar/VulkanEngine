#version 460
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
layout(location = 1) rayPayloadEXT bool o_CastShadow;

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
	vec4 Extinction_AtDistance;
    float Transmission;
	float SpecularTint;
	float IOR;
	float Sheen;
	float SheenTint;
	float Clearcoat;
	float ClearcoatGloss;
    float Anisotropy;
	float Subsurface;
} u_RTMaterialData;

#include "Utils/Lighting.glslh"

// Skybox
layout(set = 2, binding = 0) uniform sampler2D u_HDRTexture;
layout(set = 2, binding = 1) uniform sampler2D u_PDFTexture;
layout(set = 2, binding = 2) uniform sampler2D u_CDFTexture;

layout(set = 2, binding = 3) uniform SkyboxData
{
	float Intensity;
	float LOD;
} u_Skybox;

// Fresnel factor for all incidence
const vec3 Fdielectric = vec3(0.04);

struct PBRParams
{
    vec3 View;
    vec3 Normal;
    float NdotV;

    vec3 Albedo;
    vec2 Anisotropy;
    float Occlusion;
    float Metallic;
    float Roughness;
} m_Params;

vec3 GetNormalsFromMap()
{
    vec3 normal = texture(u_NormalTextureArray[nonuniformEXT(gl_InstanceCustomIndexEXT)], Input.TexCoord).xyz;
    normal.y = 1.0 - normal.y;

    vec3 tangentNormal = normalize(normal * 2.0 - 1.0);
    return normalize(Input.WorldNormals * tangentNormal);
}

#include "Utils/BSDF.glslh"

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
    vec3 position = MixBarycentric(v0.Position, v1.Position, v2.Position);
	vec3 normal = MixBarycentric(v0.Normal, v1.Normal, v2.Normal);
    vec3 tangent = MixBarycentric(v0.Tangent, v1.Tangent, v2.Tangent);
    vec3 binormal = MixBarycentric(v0.Binormal, v1.Binormal, v2.Binormal);
	vec2 texCoord = MixBarycentric(v0.TexCoord, v1.TexCoord, v2.TexCoord);

    // Set Input Data
    Input.WorldPosition = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));
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
	m_Params.Albedo = diffuse.rgb * materialData.Albedo.rgb;
    // R->Ambient Occlusion, G->Roughness, B->Metallic
    vec3 aorm = texture(u_ARMTextureArray[nonuniformEXT(gl_InstanceCustomIndexEXT)], Input.TexCoord).rgb;

    m_Params.Occlusion = aorm.r;
    m_Params.Roughness = max(aorm.g * aorm.g, 0.001) * materialData.Roughness;
    m_Params.Metallic = aorm.b * materialData.Metallic;

	vec3 cameraPosition = u_Camera.InverseView[3].xyz;
	m_Params.View = normalize(cameraPosition - Input.WorldPosition);
    m_Params.Normal = materialData.UseNormalMap == 0 ? normalize(Input.Normal) : GetNormalsFromMap();
    m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);

    float aspect = sqrt(1.0 - u_RTMaterialData.Anisotropy * 0.9);
    m_Params.Anisotropy.x = max(0.001, m_Params.Roughness / aspect);
    m_Params.Anisotropy.y = max(0.001, m_Params.Roughness * aspect);

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
    // vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metallic);

    BSDFSample bsdfSample;
	LightSample lightSample;

    if (materialData.Emission > 1.0)
	    o_RayPayload.Radiance += m_Params.Albedo * materialData.Emission * o_RayPayload.Beta;

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
		o_RayPayload.Absorption = -log(u_RTMaterialData.Extinction_AtDistance.xyz) / (u_RTMaterialData.Extinction_AtDistance.w + EPS);

	// Russian Roulette
	if (Max3(o_RayPayload.Beta) < 0.01 && o_RayPayload.Depth > 2)
	{
		float q = max(float(0.05), 1.0 - Max3(o_RayPayload.Beta));
		if (RandomFloat(o_RayPayload.Seed) < q)
			o_RayPayload.Distance = -1.0;

		o_RayPayload.Beta /= (1.0 - q);
	}

	o_RayPayload.BSDF = bsdfSample;

	// Update a new Ray Path Bounce Direction
	o_RayPayload.RayData.Direction = bsdfSample.Direction;
	o_RayPayload.RayData.Origin = Input.WorldPosition;
}
