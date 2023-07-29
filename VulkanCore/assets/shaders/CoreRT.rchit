#version 460 core
#extension GL_EXT_ray_tracing : require

struct RayPayload
{
	vec3 Color;
};

layout(location = 0) rayPayloadInEXT RayPayload o_RayPayload;

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

layout(binding = 3) readonly buffer Vertices
{
	VertexSSBOLayout v[];
} r_VertexData;

layout(binding = 4) readonly buffer Indices
{
	uint idx[];
} r_IndexData;

Vertex Unpack(int index)
{
	Vertex vertex;
	VertexSSBOLayout vLayout = r_VertexData.v[index];

	vertex.Position = vLayout.M0.xyz;
	vertex.Normal = vec3(vLayout.M0.w, vLayout.M1.xy);
	vertex.Tangent = vec3(vLayout.M1.zw, vLayout.M2.x);
	vertex.Binormal = vLayout.M2.yzw;
	vertex.Color = vLayout.M3.xyz;
	vertex.TexCoord = vec2(vLayout.M3.w, vLayout.M4.x);

	return vertex;
}

void main()
{
	ivec3 index = ivec3(r_IndexData.idx[3 * gl_PrimitiveID], r_IndexData.idx[3 * gl_PrimitiveID + 1], r_IndexData.idx[3 * gl_PrimitiveID + 2]);
	
	Vertex v0 = Unpack(index.x);
	Vertex v1 = Unpack(index.y);
	Vertex v2 = Unpack(index.z);

	o_RayPayload.Color = v2.Color;
}
