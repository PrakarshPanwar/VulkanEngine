#version 460 core

// Vertex Bindings
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec3 a_FragColor;
layout(location = 5) in vec2 a_TexCoord;
layout(location = 6) in int a_TexID;

// Instance Bindings
layout(location = 7) in vec4 a_MRow1;
layout(location = 8) in vec4 a_MRow2;
layout(location = 9) in vec4 a_MRow3;

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	mat3 WorldNormals;
	vec2 TexCoord;
	vec3 VertexColor;
};

layout(location = 0) out VertexOutput Output;
layout(location = 9) out flat int v_MaterialIndex;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InvView;
} u_Camera;

void main()
{
	mat4 transform = mat4(
		vec4(a_MRow1.x, a_MRow2.x, a_MRow3.x, 0.0),
		vec4(a_MRow1.y, a_MRow2.y, a_MRow3.y, 0.0),
		vec4(a_MRow1.z, a_MRow2.z, a_MRow3.z, 0.0),
		vec4(a_MRow1.w, a_MRow2.w, a_MRow3.w, 1.0)
	);

	vec4 positionWorld = transform * vec4(a_Position, 1.0);
	gl_Position = u_Camera.Projection * u_Camera.View * positionWorld;
	Output.Normal = mat3(transform) * a_Normal;
	Output.WorldPosition = positionWorld.xyz;
	Output.WorldNormals = mat3(transform) * mat3(a_Tangent, a_Binormal, a_Normal);
	Output.VertexColor = a_FragColor;
	Output.TexCoord = vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);
	v_MaterialIndex = a_TexID;
}