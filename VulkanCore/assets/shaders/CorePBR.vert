#version 460 core

// Vertex Bindings
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec3 a_FragColor;
layout(location = 5) in vec2 a_TexCoord;

// Instance Bindings
layout(location = 6) in vec4 a_MRow0;
layout(location = 7) in vec4 a_MRow1;
layout(location = 8) in vec4 a_MRow2;

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	mat3 WorldNormals;
	vec2 TexCoord;
	vec3 VertexColor;
};

layout(location = 0) out VertexOutput Output;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
} u_Camera;

void main()
{
	mat4 transform = mat4(
		vec4(a_MRow0.x, a_MRow1.x, a_MRow2.x, 0.0),
		vec4(a_MRow0.y, a_MRow1.y, a_MRow2.y, 0.0),
		vec4(a_MRow0.z, a_MRow1.z, a_MRow2.z, 0.0),
		vec4(a_MRow0.w, a_MRow1.w, a_MRow2.w, 1.0)
	);

	vec4 positionWorld = transform * vec4(a_Position, 1.0);
	gl_Position = u_Camera.Projection * u_Camera.View * positionWorld;
	Output.Normal = mat3(transform) * a_Normal;
	Output.WorldPosition = positionWorld.xyz;

	vec3 T = normalize(mat3(transform) * a_Tangent);
    vec3 N = normalize(mat3(transform) * a_Normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

	Output.WorldNormals = mat3(T, B, N);
	Output.VertexColor = a_FragColor;
	Output.TexCoord = vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);
}