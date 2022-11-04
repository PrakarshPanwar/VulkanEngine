#version 460 core

const vec2 Offsets[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0,  1.0),
  vec2( 1.0, -1.0),
  vec2( 1.0, -1.0),
  vec2(-1.0,  1.0),
  vec2( 1.0,  1.0)
);

layout(location = 0) out vec2 v_FragOffset;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} u_Camera;

layout(push_constant) uniform PointLight
{
	vec4 position;
	vec4 color;
	float radius;
} u_PointLight;

void main()
{
	v_FragOffset = Offsets[gl_VertexIndex];
	vec3 cameraRightWorld = { u_Camera.view[0][0], u_Camera.view[1][0], u_Camera.view[2][0] };
	vec3 cameraUpWorld = { u_Camera.view[0][1], u_Camera.view[1][1], u_Camera.view[2][1] };

	vec3 positionWorld = u_PointLight.position.xyz 
	+ u_PointLight.radius * v_FragOffset.x * cameraRightWorld
	+ u_PointLight.radius * v_FragOffset.y * cameraUpWorld;

	gl_Position = u_Camera.projection * u_Camera.view * vec4(positionWorld, 1.0);
}