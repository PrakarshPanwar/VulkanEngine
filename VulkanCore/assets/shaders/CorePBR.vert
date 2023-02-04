#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec3 a_FragColor;
layout(location = 5) in vec2 a_TexCoord;
layout(location = 6) in int a_TexID;

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

layout(push_constant) uniform Model
{
	mat4 ModelMatrix;
	mat4 NormalMatrix;
} u_Model;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InvView;
} u_Camera;

void main()
{
	vec4 positionWorld = u_Model.ModelMatrix * vec4(a_Position, 1.0);
	gl_Position = u_Camera.Projection * u_Camera.View * positionWorld;
	Output.Normal = mat3(u_Model.ModelMatrix) * a_Normal;
	Output.WorldPosition = positionWorld.xyz;
	Output.WorldNormals = mat3(u_Model.ModelMatrix) * mat3(a_Tangent, a_Binormal, a_Normal);
	Output.VertexColor = a_FragColor;
	Output.TexCoord = a_TexCoord;
	v_MaterialIndex = a_TexID;
}