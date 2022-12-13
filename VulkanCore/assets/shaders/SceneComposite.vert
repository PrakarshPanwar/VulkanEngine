#version 460 core

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out float v_Exposure;

layout(binding = 1) uniform SceneSettings
{
	float Exposure;
} u_Scene;

void main()
{
	v_Exposure = u_Scene.Exposure;
	v_TexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(v_TexCoord * 2.0 + -1.0, 0.0, 1.0);
}
