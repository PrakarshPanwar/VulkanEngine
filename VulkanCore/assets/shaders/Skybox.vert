#version 460 core

layout(location = 0) in vec3 a_Position;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
	vec2 DepthUnpackConsts;
} u_Camera;

layout(location = 0) out vec3 v_TexCoord;

void main()
{
	v_TexCoord = a_Position;
	mat4 viewMatrix = mat4(mat3(u_Camera.View));

	vec4 position = u_Camera.Projection * viewMatrix * vec4(a_Position.xyz, 1.0);
	gl_Position = position.xyww;
}