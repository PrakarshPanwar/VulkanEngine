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

layout(location = 0) out flat int o_InstanceID;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
	vec2 DepthUnpackConsts;
} u_Camera;

layout(set = 0, binding = 3) buffer EntityData
{
	int IDs[];
} s_Entities;

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

	o_InstanceID = s_Entities.IDs[gl_InstanceIndex];
}