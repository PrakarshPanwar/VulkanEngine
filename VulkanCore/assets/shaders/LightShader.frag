#version 460

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 v_TexCoord;
layout(binding = 1) uniform sampler2D u_LightTextureIcon;

void main()
{
	vec4 color = texture(u_LightTextureIcon, v_TexCoord);
	if (color.a < 0.5)
		discard;

	o_Color = color;
}