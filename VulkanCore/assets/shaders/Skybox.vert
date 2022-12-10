#version 460 core

layout(location = 0) in vec3 a_Position;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} u_Camera;

layout(location = 0) out vec3 v_TexCoord;

void main()
{
	v_TexCoord = a_Position;

	vec4 position = u_Camera.projection * u_Camera.view * vec4(a_Position.xyz, 0.1);
	gl_Position = position;
}