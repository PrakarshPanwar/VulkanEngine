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

layout(push_constant) uniform Push
{
	mat4 modelMatrix;
	mat4 normalMatrix;
	float timestep;
} push;

struct PointLight
{
	vec4 position;
	vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
	mat4 invView;
	vec4 ambientLightColor;
	PointLight pointLights[10];
	int numLights;
} ubo;

const vec2 TexCoords[4] = vec2[](
	vec2(1.0, 0.0),
	vec2(0.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

void main()
{
	vec4 positionWorld = push.modelMatrix * vec4(a_Position, 1.0);
	positionWorld.y = -positionWorld.y;
	gl_Position = ubo.projection * ubo.view * positionWorld;
	v_FragNormalWorld = normalize(mat3(push.normalMatrix) * a_Normal);
	v_FragPosWorld = positionWorld.xyz;
	v_FragColor = a_FragColor;
	v_FragTexCoord = a_TexCoord;
	v_TexIndex = a_TexID;
}