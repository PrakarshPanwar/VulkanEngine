#version 460 core

const vec2 Offsets[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

layout(location = 0) out vec2 v_FragOffset;

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

layout(push_constant) uniform Push
{
	vec4 position;
	vec4 color;
	float radius;
} push;

void main()
{
	v_FragOffset = Offsets[gl_VertexIndex];
	vec3 cameraRightWorld = { ubo.view[0][0], ubo.view[1][0], ubo.view[2][0] };
	vec3 cameraUpWorld = { ubo.view[0][1], ubo.view[1][1], ubo.view[2][1] };

	vec3 positionWorld = push.position.xyz 
	+ push.radius * v_FragOffset.x * cameraRightWorld
	+ push.radius * v_FragOffset.y * cameraUpWorld;

	gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);
	gl_Position.y = -gl_Position.y;
}