#version 460 core
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct RayPayload
{
	vec3 Color;
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

struct PointLight
{
    vec4 Position;
	vec4 Color;
    float Radius;
    float Falloff;
};

struct MeshBufferData
{
	uint64_t VertexBufferAddress;
	uint64_t IndexBufferAddress;
};

layout(binding = 3) uniform PointLightData
{
	int Count;
	PointLight PointLights[10];
} u_PointLight;

layout(binding = 4) readonly buffer MeshData
{
	MeshBufferData addressData[];
} r_MeshBufferData;

layout(buffer_reference, scalar) buffer Vertices
{
	VertexSSBOLayout v[];
} r_VertexData;

layout(buffer_reference, scalar) buffer Indices
{
	uint idx[];
} r_IndexData;

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

	// Interpolate normal
	const vec3 barycentricCoords = vec3(1.0 - g_HitAttribs.x - g_HitAttribs.y, g_HitAttribs.x, g_HitAttribs.y);
	vec3 normal = v0.Normal * barycentricCoords.x + v1.Normal * barycentricCoords.y + v2.Normal * barycentricCoords.z;

	vec3 diffuseLight = vec3(0.0);
	vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	vec3 surfaceNormal = normalize(vec3(normal * gl_WorldToObjectEXT));

	// Basic Lighting
	for (int i = 0; i < u_PointLight.Count; ++i)
	{
		PointLight pointLight = u_PointLight.PointLights[i];
		vec3 directionToLight = pointLight.Position.xyz - worldPos;
		float attentuation = 1.0 / dot(directionToLight, directionToLight);
		directionToLight = normalize(directionToLight);
		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0.2);
		vec3 intensity = pointLight.Color.xyz * pointLight.Color.w * attentuation;

		diffuseLight += intensity * cosAngIncidence;
	}

	o_RayPayload.Color = diffuseLight;
}
