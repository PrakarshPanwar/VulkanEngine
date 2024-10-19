#version 460

layout(location = 0) out vec2 v_TexCoord;

void main()
{
	v_TexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(v_TexCoord * 2.0 + -1.0, 0.0, 1.0);
}
