#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_FragColor;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in int a_TexID;

layout(location = 0) out vec3 v_FragColor;
layout(location = 1) out vec3 v_FragPosWorld;
layout(location = 2) out vec3 v_FragNormalWorld;
layout(location = 3) out vec2 v_FragTexCoord;
layout(location = 4) out flat int v_TexIndex;

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
	v_FragNormalWorld = normalize(mat3(u_Model.NormalMatrix) * a_Normal);
	v_FragPosWorld = positionWorld.xyz;
	v_FragColor = a_FragColor;
	v_FragTexCoord = a_TexCoord;
	v_TexIndex = a_TexID;
}